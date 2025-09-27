import { OrderQueue, HardwareStatus, BrewingStep } from "./queue-schemas";

// Input type for adding to queue
interface AddToQueueData {
  orderId: string;
  sessionId?: string;
  deviceId?: string;
  drinkName: string;
  toppings?: string[];
  totalAmount: number;
  size?: string;
}

// Mock Firestore for development
// ใน production จะเปลี่ยนเป็น Firestore จริง
class MockFirestore {
  private orders: Map<string, OrderQueue> = new Map();
  private hardware: Map<string, HardwareStatus> = new Map();
  private brewingSteps: Map<string, BrewingStep[]> = new Map();

  // Initialize default hardware
  constructor() {
    this.hardware.set("esp32-001", {
      hardwareId: "esp32-001",
      status: "idle",
      lastHeartbeat: new Date(),
      capabilities: {
        maxConcurrentOrders: 1,
        supportedDrinks: ["ชาไทย", "กาแฟ", "โกโก้"],
        avgBrewingTime: 45,
      },
    });
  }

  async addToQueue(
    order: Omit<OrderQueue, "id" | "queuePosition" | "estimatedTime">
  ): Promise<OrderQueue> {
    // Get current active queue (excluding completed/cancelled)
    const activeQueue = await this.getQueueStatus();
    const queuePosition = activeQueue.length + 1;
    const estimatedTime = this.calculateEstimatedTime(queuePosition);

    const queueOrder: OrderQueue = {
      ...order,
      id: `queue_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`,
      queuePosition,
      estimatedTime,
    };

    this.orders.set(queueOrder.id, queueOrder);
    
    // Reorder queue positions after adding new order
    await this.reorderQueue();
    this.notifyQueueUpdate();

    console.log(`Added to queue: ${queueOrder.id} at position ${queuePosition}`);
    return queueOrder;
  }

  async getQueueStatus(): Promise<OrderQueue[]> {
    // First, clean up and reorder the queue
    await this.reorderQueue();
    
    const activeOrders = Array.from(this.orders.values())
      .filter(
        (order) => order.status !== "completed" && order.status !== "cancelled"
      )
      .sort((a, b) => a.queuePosition - b.queuePosition);
    
    console.log(`getQueueStatus: Found ${activeOrders.length} active orders:`, activeOrders.map(o => ({
      id: o.id,
      orderId: o.orderId,
      status: o.status,
      queuePosition: o.queuePosition
    })));
    
    return activeOrders;
  }

  async getNextOrder(): Promise<OrderQueue | null> {
    const pendingOrders = Array.from(this.orders.values())
      .filter((order) => order.status === "pending")
      .sort((a, b) => a.queuePosition - b.queuePosition);

    return pendingOrders[0] || null;
  }

  async updateOrderStatus(
    orderId: string,
    status: OrderQueue["status"],
    hardwareId?: string
  ): Promise<void> {
    // Support updating by either queue entry id or the original orderId
    let targetKey: string | null = null;
    let order = this.orders.get(orderId);

    if (order) {
      targetKey = orderId;
    } else {
      for (const [key, value] of this.orders.entries()) {
        if (value.orderId === orderId || value.id === orderId) {
          order = value;
          targetKey = key;
          break;
        }
      }
    }

    if (!order || !targetKey) {
      console.log(`Order not found for update: ${orderId}`, {
        availableOrders: Array.from(this.orders.keys()).map(k => ({
          id: k,
          orderId: this.orders.get(k)?.orderId
        }))
      });
      return;
    }

    const oldStatus = order.status;
    order.status = status;

    if (status === "preparing" && !order.startedAt) {
      order.startedAt = new Date();
      if (hardwareId) order.hardwareId = hardwareId;
    }

    if (status === "completed" && !order.completedAt) {
      order.completedAt = new Date();
    }

    this.orders.set(targetKey, order);
    
    // Reorder queue when order status changes (especially when completed or started processing)
    if (status === "completed" || status === "preparing" || status === "brewing") {
      await this.reorderQueue();
    }
    
    // Always notify queue update to trigger SSE
    this.notifyQueueUpdate();
    console.log(`Updated order ${orderId} (${targetKey}): ${oldStatus} -> ${status}`, {
      orderId: order.orderId,
      queuePosition: order.queuePosition,
      hardwareId: order.hardwareId,
      status: order.status
    });
  }

  private async reorderQueue(): Promise<void> {
    const activeOrders = Array.from(this.orders.values())
      .filter(order => order.status !== "completed" && order.status !== "cancelled")
      .sort((a, b) => a.createdAt.getTime() - b.createdAt.getTime());

    console.log(`Reordering queue: ${activeOrders.length} active orders`, activeOrders.map(o => ({
      id: o.id,
      orderId: o.orderId,
      status: o.status,
      queuePosition: o.queuePosition
    })));

    // Reassign queue positions
    activeOrders.forEach((order, index) => {
      const newPosition = index + 1;
      order.queuePosition = newPosition;
      order.estimatedTime = this.calculateEstimatedTime(newPosition);
      this.orders.set(order.id, order);
      console.log(`Updated order ${order.id}: position ${newPosition}, estimatedTime ${order.estimatedTime}`);
    });

    // Clean up old completed orders (older than 30 minutes)
    const thirtyMinutesAgo = new Date(Date.now() - 30 * 60 * 1000);
    const completedOrders = Array.from(this.orders.values())
      .filter(order => order.status === "completed" && order.completedAt && order.completedAt < thirtyMinutesAgo);
    
    completedOrders.forEach(order => {
      this.orders.delete(order.id);
    });

    // Also clean up very old orders (older than 2 hours) regardless of status
    const twoHoursAgo = new Date(Date.now() - 2 * 60 * 60 * 1000);
    const oldOrders = Array.from(this.orders.values())
      .filter(order => order.createdAt < twoHoursAgo);
    
    oldOrders.forEach(order => {
      this.orders.delete(order.id);
    });

    if (completedOrders.length > 0 || oldOrders.length > 0) {
      console.log(`Cleaned up ${completedOrders.length} completed orders and ${oldOrders.length} old orders`);
    }

    console.log(`Reordered queue: ${activeOrders.length} active orders`);
  }

  async updateHardwareStatus(
    hardwareId: string,
    status: HardwareStatus["status"],
    currentOrderId?: string
  ): Promise<void> {
    const hardware = this.hardware.get(hardwareId);
    if (!hardware) return;

    hardware.status = status;
    hardware.currentOrderId = currentOrderId;
    hardware.lastHeartbeat = new Date();

    this.hardware.set(hardwareId, hardware);
  }

  async addBrewingStep(step: BrewingStep): Promise<void> {
    const steps = this.brewingSteps.get(step.orderId) || [];
    steps.push(step);
    this.brewingSteps.set(step.orderId, steps);
    this.notifyBrewingUpdate(step.orderId, step);
  }

  async getBrewingSteps(orderId: string): Promise<BrewingStep[]> {
    return this.brewingSteps.get(orderId) || [];
  }

  async getOrderByOrderId(orderId: string): Promise<OrderQueue | null> {
    // Try direct match on queue id first
    const byQueueId = this.orders.get(orderId);
    if (byQueueId) return byQueueId;

    // Then find by original orderId
    for (const value of this.orders.values()) {
      if (value.orderId === orderId) {
        return value;
      }
    }
    return null;
  }

  async clearAllOrders(): Promise<void> {
    const orderCount = this.orders.size;
    this.orders.clear();
    console.log(`Cleared all ${orderCount} orders from queue`);
    this.notifyQueueUpdate();
  }

  private getQueueLength(): number {
    return Array.from(this.orders.values()).filter(
      (order) =>
        order.status === "pending" ||
        order.status === "preparing" ||
        order.status === "brewing"
    ).length;
  }

  private calculateEstimatedTime(queuePosition: number): number {
    const avgBrewingTime = 3; // 3 นาที per order
    return (queuePosition - 1) * avgBrewingTime;
  }

  private notifyQueueUpdate(): void {
    // Emit event for WebSocket clients
    if (typeof window !== "undefined") {
      window.dispatchEvent(
        new CustomEvent("queueUpdate", {
          detail: { queue: this.getQueueStatus() },
        })
      );
    }
  }

  private notifyBrewingUpdate(orderId: string, step: BrewingStep): void {
    if (typeof window !== "undefined") {
      window.dispatchEvent(
        new CustomEvent("brewingUpdate", {
          detail: { orderId, step },
        })
      );
    }
  }
}

export const queueService = new MockFirestore();

// Zustand store สำหรับ Queue
import { create } from "zustand";

interface QueueStore {
  queue: OrderQueue[];
  currentUserOrder: OrderQueue | null;
  isLoading: boolean;

  addToQueue: (orderData: AddToQueueData) => Promise<OrderQueue>;
  updateQueue: (queue: OrderQueue[]) => void;
  setCurrentUserOrder: (order: OrderQueue | null) => void;
  refreshQueue: () => Promise<void>;
}

export const useQueueStore = create<QueueStore>((set) => ({
  queue: [],
  currentUserOrder: null,
  isLoading: false,

  addToQueue: async (orderData) => {
    set({ isLoading: true });

    const queueOrder = await queueService.addToQueue({
      orderId: orderData.orderId,
      status: "pending",
      createdAt: new Date(),
      customer: {
        sessionId: orderData.sessionId || "anonymous",
        deviceId: orderData.deviceId,
      },
      order: {
        drinkName: orderData.drinkName,
        toppings: orderData.toppings || [],
        totalAmount: orderData.totalAmount,
        size: orderData.size || "Regular",
      },
    });

    set({
      currentUserOrder: queueOrder,
      isLoading: false,
    });

    return queueOrder;
  },

  updateQueue: (queue) => set({ queue }),

  setCurrentUserOrder: (order) => set({ currentUserOrder: order }),

  refreshQueue: async () => {
    const queue = await queueService.getQueueStatus();
    set({ queue });
  },
}));

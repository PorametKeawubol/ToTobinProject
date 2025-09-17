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
    const queuePosition = this.getQueueLength() + 1;
    const estimatedTime = this.calculateEstimatedTime(queuePosition);

    const queueOrder: OrderQueue = {
      ...order,
      id: `queue_${Date.now()}`,
      queuePosition,
      estimatedTime,
    };

    this.orders.set(queueOrder.id, queueOrder);
    this.notifyQueueUpdate();

    return queueOrder;
  }

  async getQueueStatus(): Promise<OrderQueue[]> {
    return Array.from(this.orders.values())
      .filter(
        (order) => order.status !== "completed" && order.status !== "cancelled"
      )
      .sort((a, b) => a.queuePosition - b.queuePosition);
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
    const order = this.orders.get(orderId);
    if (!order) return;

    order.status = status;

    if (status === "preparing" && !order.startedAt) {
      order.startedAt = new Date();
      if (hardwareId) order.hardwareId = hardwareId;
    }

    if (status === "completed" && !order.completedAt) {
      order.completedAt = new Date();
    }

    this.orders.set(orderId, order);
    this.notifyQueueUpdate();
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

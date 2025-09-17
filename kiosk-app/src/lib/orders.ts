import type { Order, OrderOptions } from "./schemas";

// In-memory storage for orders (replace with database in production)
const orderStore = new Map<string, Order>();

export const Orders = {
  create(drinkId: string, options: OrderOptions, amount: number): Order {
    const id = `order_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;

    const order: Order = {
      id,
      drinkId,
      options,
      amount,
      status: "IDLE",
      createdAt: Date.now(),
    };

    orderStore.set(id, order);
    return order;
  },

  get(orderId: string): Order | undefined {
    return orderStore.get(orderId);
  },

  update(orderId: string, updates: Partial<Order>): Order | null {
    const order = orderStore.get(orderId);
    if (!order) return null;

    const updatedOrder = { ...order, ...updates };
    orderStore.set(orderId, updatedOrder);
    return updatedOrder;
  },

  updateStatus(orderId: string, status: Order["status"]): Order | null {
    const order = orderStore.get(orderId);
    if (!order) return null;

    const updates: Partial<Order> = { status };

    // Set timestamps based on status
    if (status === "PAID" && !order.paidAt) {
      updates.paidAt = Date.now();
    } else if (status === "DONE" && !order.finishedAt) {
      updates.finishedAt = Date.now();
    }

    return this.update(orderId, updates);
  },

  delete(orderId: string): boolean {
    return orderStore.delete(orderId);
  },

  getAll(): Order[] {
    return Array.from(orderStore.values());
  },

  // Get orders by status
  getByStatus(status: Order["status"]): Order[] {
    return this.getAll().filter((order) => order.status === status);
  },

  // Clean up old orders (can be called periodically)
  cleanup(olderThanMs: number = 24 * 60 * 60 * 1000): number {
    const cutoff = Date.now() - olderThanMs;
    let cleaned = 0;

    for (const [id, order] of orderStore.entries()) {
      if (order.createdAt < cutoff && order.status === "DONE") {
        orderStore.delete(id);
        cleaned++;
      }
    }

    return cleaned;
  },
};

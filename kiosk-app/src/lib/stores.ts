import { create } from "zustand";
import type { Drink, Topping, OrderOptions, Order } from "@/lib/schemas";

// Single item store for kiosk ordering (no cart/multiple items)
interface CurrentItemStore {
  currentItem: {
    drink: Drink;
    options: OrderOptions;
    amount: number;
  } | null;

  // Actions
  setCurrentItem: (drink: Drink, options: OrderOptions, amount: number) => void;
  clearCurrentItem: () => void;
  clearCart: () => void; // Backward compatibility alias
}

// Note: Named "useCartStore" for backward compatibility, but it only handles single current item
export const useCartStore = create<CurrentItemStore>((set) => ({
  currentItem: null,

  setCurrentItem: (drink, options, amount) => {
    set({ currentItem: { drink, options, amount } });
  },

  clearCurrentItem: () => {
    set({ currentItem: null });
  },

  clearCart: () => {
    set({ currentItem: null });
  },
}));

interface OrderStore {
  currentOrder: Order | null;
  orderHistory: Order[];

  // Actions
  setCurrentOrder: (order: Order) => void;
  updateCurrentOrder: (updates: Partial<Order>) => void;
  clearCurrentOrder: () => void;
  addToHistory: (order: Order) => void;
  getOrderById: (orderId: string) => Order | undefined;
}

export const useOrderStore = create<OrderStore>((set, get) => ({
  currentOrder: null,
  orderHistory: [],

  setCurrentOrder: (order) => {
    set({ currentOrder: order });
  },

  updateCurrentOrder: (updates) => {
    set((state) => ({
      currentOrder: state.currentOrder
        ? { ...state.currentOrder, ...updates }
        : null,
    }));
  },

  clearCurrentOrder: () => {
    set({ currentOrder: null });
  },

  addToHistory: (order) => {
    set((state) => ({
      orderHistory: [order, ...state.orderHistory.slice(0, 49)], // Keep last 50 orders
    }));
  },

  getOrderById: (orderId) => {
    const { currentOrder, orderHistory } = get();
    if (currentOrder?.id === orderId) return currentOrder;
    return orderHistory.find((order) => order.id === orderId);
  },
}));

interface KioskStore {
  isLocked: boolean;
  lockedOrderId: string | null;

  // Actions
  setLocked: (locked: boolean, orderId?: string) => void;
  unlock: () => void;
}

export const useKioskStore = create<KioskStore>((set) => ({
  isLocked: false,
  lockedOrderId: null,

  setLocked: (locked, orderId) => {
    set({
      isLocked: locked,
      lockedOrderId: locked ? orderId || null : null,
    });
  },

  unlock: () => {
    set({ isLocked: false, lockedOrderId: null });
  },
}));

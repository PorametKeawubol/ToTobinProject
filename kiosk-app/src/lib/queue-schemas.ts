import { z } from "zod";

// Order Queue Schema
export const OrderQueueSchema = z.object({
  id: z.string(),
  orderId: z.string(),
  queuePosition: z.number(),
  status: z.enum([
    "pending", // รอคิว
    "preparing", // กำลังทำ
    "brewing", // กำลังชง
    "completed", // เสร็จแล้ว
    "cancelled", // ยกเลิก
  ]),
  estimatedTime: z.number(), // นาที
  createdAt: z.date(),
  startedAt: z.date().optional(),
  completedAt: z.date().optional(),
  hardwareId: z.string().optional(),
  customer: z.object({
    sessionId: z.string(),
    deviceId: z.string().optional(),
  }),
  order: z.object({
    drinkName: z.string(),
    toppings: z.array(z.string()),
    totalAmount: z.number(),
    size: z.string().default("Regular"),
  }),
});

// Hardware Status Schema
export const HardwareStatusSchema = z.object({
  hardwareId: z.string(),
  status: z.enum([
    "idle", // ว่าง
    "busy", // กำลังทำ
    "maintenance", // ปรับปรุง
    "offline", // ออฟไลน์
  ]),
  currentOrderId: z.string().optional(),
  lastHeartbeat: z.date(),
  capabilities: z.object({
    maxConcurrentOrders: z.number().default(1),
    supportedDrinks: z.array(z.string()),
    avgBrewingTime: z.number(), // วินาที
  }),
});

// Brewing Step Status
export const BrewingStepSchema = z.object({
  orderId: z.string(),
  step: z.enum([
    "preparing_cup",
    "adding_toppings",
    "adding_ice",
    "brewing_drink",
    "completed",
  ]),
  status: z.enum(["pending", "in_progress", "completed", "error"]),
  timestamp: z.date(),
  message: z.string().optional(),
  hardwareId: z.string(),
});

export type OrderQueue = z.infer<typeof OrderQueueSchema>;
export type HardwareStatus = z.infer<typeof HardwareStatusSchema>;
export type BrewingStep = z.infer<typeof BrewingStepSchema>;

import { z } from "zod";

export const Topping = z.object({
  id: z.string(),
  name: z.string(),
  price: z.number(),
});

export const Drink = z.object({
  id: z.string(),
  name: z.string(),
  basePrice: z.number(),
  image: z.string().optional(),
  description: z.string().optional(),
});

export const OrderOptions = z.object({
  size: z.enum(["S", "M", "L"]),
  sweetness: z.number().min(0).max(100),
  toppings: z.array(Topping),
});

export const Order = z.object({
  id: z.string(),
  drinkId: z.string(),
  options: OrderOptions,
  amount: z.number(),
  status: z.enum([
    "IDLE",
    "AWAITING_PAYMENT",
    "PAID",
    "DISPENSING",
    "DONE",
    "CANCELLED",
    "ERROR",
  ]),
  createdAt: z.number(),
  paidAt: z.number().optional(),
  finishedAt: z.number().optional(),
});

export const PaymentRequest = z.object({
  orderId: z.string(),
  amount: z.number(),
});

export const PaymentStatus = z.enum([
  "PENDING",
  "PAID",
  "EXPIRED",
  "CANCELLED",
]);

export const CreatePaymentResult = z.object({
  provider: z.literal("mock"),
  orderId: z.string(),
  amount: z.number(),
  qrImageDataUrl: z.string(),
  expiresInSec: z.number(),
});

// Type exports
export type Topping = z.infer<typeof Topping>;
export type Drink = z.infer<typeof Drink>;
export type OrderOptions = z.infer<typeof OrderOptions>;
export type Order = z.infer<typeof Order>;
export type PaymentRequest = z.infer<typeof PaymentRequest>;
export type PaymentStatus = z.infer<typeof PaymentStatus>;
export type CreatePaymentResult = z.infer<typeof CreatePaymentResult>;

// Size pricing multipliers
export const SIZE_MULTIPLIERS = {
  S: 1.0,
  M: 1.3,
  L: 1.6,
} as const;

// Sample data
export const SAMPLE_DRINKS: Drink[] = [
  {
    id: "thai-tea",
    name: "ชาไทย",
    basePrice: 45,
    description: "ชาไทยสูตรดั้งเดิม หอมหวานมัน",
  },
  {
    id: "green-tea",
    name: "ชาเขียว",
    basePrice: 40,
    description: "ชาเขียวญี่ปุ่น เข้มข้น สดชื่น",
  },
  {
    id: "coffee",
    name: "กาแฟ",
    basePrice: 50,
    description: "กาแฟคั่วกลาง หอมกรุ่น",
  },
  {
    id: "chocolate",
    name: "โกโก้",
    basePrice: 48,
    description: "โกโก้เข้มข้น หวานหอม",
  },
  {
    id: "milk-tea",
    name: "ชานม",
    basePrice: 42,
    description: "ชานมไต้หวัน นุ่มนวล",
  },
  {
    id: "fruit-tea",
    name: "ชาผลไม้",
    basePrice: 55,
    description: "ชาผลไม้รวม สดชื่น",
  },
];

export const SAMPLE_TOPPINGS: Topping[] = [
  { id: "pearl", name: "ไข่มุก", price: 10 },
  { id: "jelly", name: "เจลลี่", price: 8 },
  { id: "pudding", name: "พุดดิ้ง", price: 12 },
  { id: "grass-jelly", name: "เฉาก๊วย", price: 10 },
  { id: "coconut", name: "มะพร้าวอ่อน", price: 15 },
  { id: "red-bean", name: "ถั่วแดง", price: 10 },
];

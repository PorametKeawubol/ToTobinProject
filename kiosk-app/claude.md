# Prompt: Generate a lightweight Kiosk (ตู้เต่าบิน) app with Next.js (frontend-first)

> ใช้เอกสารนี้เป็น **พรอมป์ต์** เพื่อให้ AI สร้างโปรเจกต์ Next.js แบบเบา โฟกัส UX/UI สวยลื่น ใช้งานบน Odroid C4 โหมดคีออสก์ มีการสั่งเมนู → ชำระเงินด้วย QR ภายใน 60 วินาที → ล็อกคิวระหว่างการชง → หน้าสถานะงาน → เสร็จแล้วกลับพร้อมรับคำสั่งใหม่ โดย **ยุบ backend ให้อยู่ใน Next.js (Route Handlers)** เท่าที่จำเป็น (ไม่มีเซิร์ฟเวอร์แยก)

## 1) เป้าหมาย (Goals)

* คีออสก์ขายเครื่องดื่มแบบ “ตู้เต่าบิน” ที่สั่งบนจอทัช
* โฟลว์: Landing/โฆษณา → เลือกเมนู/คัสตอม → Checkout (QR + timer 60s) → In-Progress (ล็อคเครื่อง) → Done
* สวย ลื่น โหลดไว ใช้งานออฟไลน์ชั่วคราวได้ (PWA)
* Back-end ให้เบาสุด: ใช้ **Next.js Route Handlers** + in-memory/SQLite (ถ้าจำเป็น)
* สามารถต่อยอดภายหลังให้คุยกับ ESP32 ผ่าน MQTT/HTTP ได้ (ใส่ stub ไว้)

## 2) Constraints & Non-goals

* ไม่ทำระบบหลังบ้านซับซ้อน (ไม่มี microservices)
* การจ่ายเงินจริงให้ทำ **อินเทอร์เฟซ** + **mock provider** พร้อม Hook/Callback เพื่อแทนที่ภายหลัง
* โครงการเน้น **UI/UX** และ **Performance** ก่อน

## 3) Tech Stack

* **Next.js 14+ (App Router)** + **TypeScript**
* **Tailwind CSS** + **shadcn/ui** (Button, Card, Dialog, Sheet, Toast)
* **Zustand** (local state: cart/order) + **TanStack Query** (API calls)
* **next-pwa** (PWA + offline fallback)
* **Route Handlers** (`app/api/**/route.ts`) สำหรับ: orders, payments, status, kiosk-lock
* **(Optional) SQLite** ด้วย `better-sqlite3` หรือ `drizzle` (ถ้าต้องเก็บประวัติ)

## 4) โครงสร้างโฟลเดอร์ (ให้ AI สร้าง)

```
/app
  /(kiosk)
    /layout.tsx
    /page.tsx                      // Landing + โฆษณา carousel
    /menu/page.tsx                 // เลือกเมนู/ขนาด/ความหวาน/ท็อปปิ้ง
    /checkout/page.tsx             // QR + 60s timer + ปุ่มยกเลิก
    /in-progress/page.tsx          // แสดงสถานะ “กำลังทำแก้วนี้”
    /done/page.tsx                 // ใบเสร็จ/ขอบคุณ
  /api
    /payments
      /create/route.ts             // POST สร้างคำสั่งจ่าย -> คืน QR payload/image
      /mock-webhook/route.ts       // POST ฝั่ง mock provider ยิงกลับสถานะจ่ายสำเร็จ
      /status/[orderId]/route.ts   // GET เช็คสถานะชำระเงิน
    /orders
      /create/route.ts             // POST เปิดออเดอร์ + ใส่ตัวเลือก
      /status/[orderId]/route.ts   // GET สถานะออเดอร์ (IDLE/PAID/DISPENSING/DONE/ERROR)
      /lock/route.ts               // POST/DELETE ตั้ง/ปลดล็อกเครื่อง (single-order lock)
    /device
      /start/route.ts              // POST (stub) สั่งเริ่มชง (แทนที่ด้วย MQTT/HTTP ภายหลัง)
      /heartbeat/route.ts          // GET (stub) ตรวจสุขภาพ
/components
  AdCarousel.tsx
  MenuGrid.tsx
  OptionSelectors.tsx (size/sweetness/toppings)
  PriceBar.tsx
  QRPanel.tsx (แสดง QR + timer 60s)
  ProgressPanel.tsx
  LockedBanner.tsx
  TopBar.tsx / BottomNav.tsx
/lib
  payments.ts (interface + mock provider)
  orders.ts (in-memory store หรือ SQLite adapter)
  kioskLock.ts
  device.ts (stub control + later: MQTT client)
  schemas.ts (zod schemas & types)
  utils.ts
/styles (globals.css, tailwind.css)
/public (icons, manifest.json, offline.html)
```

## 5) UX/UI Spec (ให้ AI ทำตามอย่างละเอียด)

* **ธีม**: modern-minimal, เน้นปุ่มใหญ่/อ่านง่าย, รองรับจอทัช 1080p แนวนอน
* **สี**: พื้นหลังสว่างนวล, Primary คุมโทนเขียว-ฟ้า/มินต์, Accent เหลืองอ่อน (ให้ AI กำหนดโค้ดสีใน Tailwind theme)
* **ตัวอักษร**: Inter/Noto Sans Thai, ขนาดใหญ่ (base ≥ 18px), ปุ่ม ≥ 56px สูง
* **Animation**: framer-motion เบา ๆ (fade/slide) ≤ 200ms
* **Carousel โฆษณา**: auto-rotate, กดแล้วไป `/menu` พร้อมติดพารามิเตอร์แคมเปญ (utm)
* **Menu**: การ์ดเครื่องดื่ม + ราคา, แตะแล้วเข้า Option (sweetness %, toppings multi-select)
* **Checkout**: แสดงสรุป + QR + ตัวจับเวลา 60s (นับถอยหลังแบบชัดเจน) + ปุ่ม “ยกเลิก”
* **In-Progress**: progress bar/animation + ข้อความ “กำลังทำแก้วนี้ • กรุณารอสักครู่” และปิดอินพุตทั้งหมด
* **Done**: หน้ายืนยัน/ใบเสร็จย่อ + ปุ่ม “กลับหน้าแรก” (auto-redirect 5–8s)
* **Accessibility**: Contrast AA+, focus states, hit area ใหญ่, no tiny text

## 6) ฟลว์สถานะ (State Machine)

```
IDLE → (create order) → AWAITING_PAYMENT (60s)
AWAITING_PAYMENT → (paid webhook) → PAID → (device/start) → DISPENSING → DONE → IDLE
AWAITING_PAYMENT → (timeout/cancel) → CANCELLED → IDLE
DISPENSING → (error) → ERROR → IDLE
```

* เก็บ `orderId`, `amount`, `recipe`, `options`, `status`, `startedAt`, `paidAt`, `finishedAt`

## 7) Data Models & Types (ตัวอย่างให้ AI ยึด)

```ts
// /lib/schemas.ts
import { z } from "zod";
export const Topping = z.object({ id: z.string(), name: z.string(), price: z.number() });
export const Drink = z.object({ id: z.string(), name: z.string(), basePrice: z.number(), image: z.string().optional() });
export const OrderOptions = z.object({ size: z.enum(["S","M","L"]), sweetness: z.number().min(0).max(100), toppings: z.array(Topping) });
export const Order = z.object({ id: z.string(), drinkId: z.string(), options: OrderOptions, amount: z.number(), status: z.enum(["IDLE","AWAITING_PAYMENT","PAID","DISPENSING","DONE","CANCELLED","ERROR"]), createdAt: z.number(), paidAt: z.number().optional(), finishedAt: z.number().optional() });
export type Order = z.infer<typeof Order>;
```

## 8) Payments Interface (mock + แทนที่ได้)

```ts
// /lib/payments.ts
export type PaymentStatus = "PENDING" | "PAID" | "EXPIRED" | "CANCELLED";
export type CreatePaymentResult = { provider: "mock"; orderId: string; amount: number; qrImageDataUrl: string; expiresInSec: number };

export const Payments = {
  async create(orderId: string, amount: number): Promise<CreatePaymentResult> {
    // TODO: generate PromptPay-like QR (mock): return data URL + expiresInSec=60
    return { provider: "mock", orderId, amount, qrImageDataUrl: "data:image/png;base64,....", expiresInSec: 60 };
  },
  async status(orderId: string): Promise<PaymentStatus> {
    // TODO: look up in memory map; default PENDING
    return "PENDING";
  },
};
```

* ให้ทำ **/api/payments/create** → เรียก `Payments.create`
* ให้ทำ **/api/payments/status/\[orderId]** → คืนสถานะ PENDING/PAID/EXPIRED
* ให้มี **/api/payments/mock-webhook** → เปลี่ยนสถานะเป็น PAID เมื่อยิงด้วย `{orderId}` (ใช้ทดสอบแทน provider จริง)

## 9) Single-Order Lock (กันรับซ้อนขณะกำลังชง)

* ตัวแปร in-memory `isLocked: boolean` + `lockedOrderId?: string`
* Endpoint: `POST /api/orders/lock {orderId}` → ถ้าไม่ได้ล็อก ให้ล็อกและคืน `200`
* `DELETE /api/orders/lock` → ปลดล็อกเมื่อจบงาน
* UI: ถ้า `isLocked` เป็น true ให้หน้า Landing/เมนูปิดการกดสั่งและแสดง `LockedBanner`

## 10) Device Control Stub (ต่อ ESP32 ภายหลัง)

* `POST /api/device/start {orderId}` → ตอนนี้เป็น mock: delay 15–30s แล้วตั้งสถานะออเดอร์เป็น `DONE` (หรือสุ่ม ERROR)
* ภายหลังให้แทนที่ด้วย MQTT/HTTP ไปหา ESP32

## 11) เชื่อมต่อหน้า UI ↔ API (ให้ AI ทำ)

* `/menu` → เพิ่มลงตะกร้า/คำนวณราคา → `POST /api/orders/create`
* `/checkout` → `POST /api/payments/create` รับ `qrImageDataUrl` + เริ่ม timer 60s

  * Poll `GET /api/payments/status/[orderId]` ทุก 2s **หรือ** ใช้ SSE/WebSocket (optional)
  * ถ้า `PAID` → Call `POST /api/device/start` + `POST /api/orders/lock`
  * ถ้า timeout → ยกเลิก/ลบออเดอร์ → กลับหน้าแรก
* `/in-progress` → Poll `GET /api/orders/status/[orderId]`

  * ถ้า `DONE`|`ERROR` → `DELETE /api/orders/lock` → ไป `/done`

## 12) Performance Budget & PWA

* FCP < 1.5s, LCP < 2.5s บน Odroid C4
* Code-splitting, dynamic import สำหรับหน้าย่อย (เช่น OptionSelectors)
* Preload font, compress images (next/image)
* PWA: cache หน้า landing + assets + offline fallback (`/offline`)

## 13) Styling Guidelines

* ใช้ Tailwind + shadcn/ui, radii-2xl, shadow soft, spacing กว้าง (p-6 ขึ้นไป)
* ปุ่มหลักใหญ่ (w-full, h-14, text-lg/xl)
* ใช้ CSS variables สำหรับ theme (สีพื้น, กดแล้ว, disabled)

## 14) Testing & DX

* ใส่ Playwright test เบื้องต้น:

  * เปิดแอป → กดโฆษณา → เลือกเมนู → Checkout → เห็น QR + timer → ยิง mock-webhook → เปลี่ยนหน้าสถานะ → Done
* Lint/Format: ESLint + Prettier

## 15) Env & Config

* `.env.local` ตัวอย่าง:

```
KIOSK_TITLE="TurtleBin Kiosk"
KIOSK_LOCK_TIMEOUT_SEC=120
PAYMENT_PROVIDER="mock"
```

## 16) Acceptance Criteria (ให้ AI ตรวจ checklist ก่อนส่งงาน)

* [ ] หน้าทั้ง 5: Landing, Menu, Checkout (มี QR + 60s), In-Progress, Done
* [ ] โฆษณาหมุนและกดแล้วไปเมนูได้
* [ ] ระบบตะกร้า/คัสตอม (size, sweetness, toppings) คิดราคาได้
* [ ] API mock ครบ: orders, payments (create/status/mock-webhook), lock, device/start
* [ ] Single-order lock ทำงาน: ระหว่าง DISPENSING สั่งใหม่ไม่ได้
* [ ] Transition ลื่น, ปุ่มใหญ่ ใช้งานทัชง่าย
* [ ] PWA ใช้งาน offline fallback ได้
* [ ] โค้ด TypeScript + zod schemas + แยก component ชัดเจน

## 17) คำสั่งที่ให้ AI ทำต่อทันที

1. สร้างโปรเจกต์ Next.js (App Router + TS) พร้อม Tailwind, shadcn/ui, next-pwa
2. สร้างโฟลเดอร์ตามข้อ 4 พร้อมไฟล์ skeleton ทุกตัว
3. เติม UI ตาม UX Spec + Theme + Animation ตามข้อ 5
4. เติม Route Handlers + in-memory stores สำหรับ orders/payments/lock
5. ใส่ mock QR generator (data URL) และ mock-webhook เพื่อทดสอบการจ่ายสำเร็จ
6. ทำ flow ครบทั้ง 5 หน้าให้ใช้งานได้ end-to-end (ไม่พึ่งเซิร์ฟเวอร์ภายนอก)
7. เพิ่ม Playwright test สคริปต์ตามข้อ 14

---

**หมายเหตุ**: อย่าลืมทำโหมดคีออสก์ (Chromium --kiosk) และ auto-restart (PM2/systemd) ใน README.

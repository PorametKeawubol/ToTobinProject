#!/usr/bin/env node
/*
  Simple Hardware Simulator for TotoBin Kiosk
  - Polls /api/hardware/orders for next job
  - Reports progress to /api/hardware/progress
  - Sends heartbeat via /api/hardware/orders poll
*/

const fetch = global.fetch || (await import("node-fetch")).default;

const BASE_URL = process.env.BASE_URL || "https://porametix.online";
const HARDWARE_ID = process.env.HARDWARE_ID || "esp32-001";
const API_KEY =
  process.env.HARDWARE_API_KEY || "odroid-hardware-key-1758367749";

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

async function pollNextOrder() {
  const url = `${BASE_URL}/api/hardware/orders?hardwareId=${encodeURIComponent(
    HARDWARE_ID
  )}`;
  const res = await fetch(url, {
    headers: { "X-API-Key": API_KEY },
  });
  if (!res.ok) throw new Error(`Poll orders failed: ${res.status}`);
  return res.json();
}

async function postProgress(payload) {
  const url = `${BASE_URL}/api/hardware/progress`;
  const res = await fetch(url, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  if (!res.ok) {
    // Fallback: try /api/hardware/status with richer body and API key
    const fallback = await fetch(`${BASE_URL}/api/hardware/status`, {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        "X-API-Key": API_KEY,
      },
      body: JSON.stringify({
        orderId: payload.orderId,
        status: payload.step === "completed" ? "completed" : "brewing",
        step: payload.step,
        message: payload.message,
        hardwareId: payload.hardwareId,
        ledState: {
          preparing: payload.step === "preparing_cup",
          toppings: payload.step === "adding_toppings",
          ice: payload.step === "adding_ice",
          brewing: payload.step === "brewing_drink",
          completed: payload.step === "completed",
        },
        progress: 0,
      }),
    });
    if (!fallback.ok) {
      throw new Error(`Progress post failed: ${res.status} / fallback: ${fallback.status}`);
    }
  }
}

async function brew(orderId) {
  const steps = [
    { step: "preparing_cup", message: "กำลังเตรียมแก้ว" },
    { step: "adding_toppings", message: "กำลังใส่ท็อปปิ้ง" },
    { step: "adding_ice", message: "กำลังใส่น้ำแข็ง" },
    { step: "brewing_drink", message: "กำลังชงเครื่องดื่ม" },
    { step: "completed", message: "เสร็จสิ้น" },
  ];

  for (const s of steps) {
    console.log(`[sim] ${orderId} → ${s.step}`);
    await postProgress({
      orderId,
      hardwareId: HARDWARE_ID,
      step: s.step,
      status: s.step === "completed" ? "completed" : "in_progress",
      message: s.message,
    });
    if (s.step !== "completed") {
      await sleep(10000); // 10s per state for faster testing
    }
  }
}

async function main() {
  console.log(`[sim] Starting hardware sim ${HARDWARE_ID} @ ${BASE_URL}`);
  while (true) {
    try {
      const data = await pollNextOrder();
      if (data?.order?.orderId) {
        console.log(`[sim] Received order ${data.order.orderId} (#${data.order.queuePosition})`);
        await brew(data.order.orderId);
        console.log(`[sim] Completed order ${data.order.orderId}`);
      } else {
        // No jobs; wait a bit
        await sleep(2000); // Faster polling
      }
    } catch (err) {
      console.error("[sim] Error:", err.message);
      await sleep(5000);
    }
  }
}

main();
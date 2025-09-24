#!/usr/bin/env node
/*
  Test script for queue system
  - Creates test orders
  - Checks queue status
  - Simulates hardware processing
*/

const fetch = global.fetch || (await import("node-fetch")).default;

const BASE_URL = process.env.BASE_URL || "http://localhost:3000";

async function createTestOrder(orderNum) {
  console.log(`\n=== Creating Test Order ${orderNum} ===`);
  
  try {
    // Create order
    const orderResponse = await fetch(`${BASE_URL}/api/orders/create`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        drinkId: "thai-tea",
        options: {
          size: "M",
          sweetness: 50,
          toppings: [{ id: "jelly", name: "เจลลี่", price: 5 }]
        },
        amount: 55
      }),
    });

    if (!orderResponse.ok) {
      throw new Error(`Order creation failed: ${orderResponse.status}`);
    }

    const order = await orderResponse.json();
    console.log(`✅ Order created: ${order.id}`);

    // Create payment
    const paymentResponse = await fetch(`${BASE_URL}/api/payments/create`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        orderId: order.id,
        amount: order.amount,
      }),
    });

    if (!paymentResponse.ok) {
      throw new Error(`Payment creation failed: ${paymentResponse.status}`);
    }

    const payment = await paymentResponse.json();
    console.log(`✅ Payment created: ${payment.orderId}`);

    // Mock payment success
    const webhookResponse = await fetch(`${BASE_URL}/api/payments/mock-webhook`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ orderId: order.id, status: "PAID" }),
    });

    if (!webhookResponse.ok) {
      throw new Error(`Webhook failed: ${webhookResponse.status}`);
    }

    console.log(`✅ Payment marked as paid`);

    // Add to queue
    const queueResponse = await fetch(`${BASE_URL}/api/queue`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        orderId: order.id,
        sessionId: `test-session-${orderNum}`,
        drinkName: `ชาไทย (ทดสอบ ${orderNum})`,
        toppings: ["เจลลี่"],
        totalAmount: order.amount,
        size: "Regular",
      }),
    });

    if (!queueResponse.ok) {
      throw new Error(`Queue add failed: ${queueResponse.status}`);
    }

    const queueData = await queueResponse.json();
    console.log(`✅ Added to queue: position ${queueData.queueOrder.queuePosition}`);

    return {
      orderId: order.id,
      queueId: queueData.queueOrder.id,
      position: queueData.queueOrder.queuePosition
    };

  } catch (error) {
    console.error(`❌ Error creating test order ${orderNum}:`, error.message);
    return null;
  }
}

async function checkQueueStatus() {
  console.log(`\n=== Queue Status ===`);
  
  try {
    const response = await fetch(`${BASE_URL}/api/queue`);
    const data = await response.json();
    
    if (data.success) {
      console.log(`Total in queue: ${data.totalInQueue}`);
      data.queue.forEach((order, index) => {
        console.log(`${index + 1}. ${order.orderId.slice(-8)} - ${order.status} - Position: ${order.queuePosition}`);
      });
    } else {
      console.log("❌ Failed to get queue status");
    }
  } catch (error) {
    console.error("❌ Error checking queue:", error.message);
  }
}

async function simulateHardwareProcessing() {
  console.log(`\n=== Simulating Hardware Processing ===`);
  
  try {
    const response = await fetch(`${BASE_URL}/api/hardware/orders?hardwareId=test-hardware`, {
      headers: { "X-API-Key": "dev-hardware-key" }
    });
    
    const data = await response.json();
    
    if (data.success && data.order) {
      console.log(`✅ Hardware got order: ${data.order.orderId}`);
      
      // Simulate brewing steps
      const steps = [
        { step: "preparing_cup", message: "กำลังเตรียมแก้ว" },
        { step: "adding_toppings", message: "ใส่ท็อปปิ้ง" },
        { step: "adding_ice", message: "ใส่น้ำแข็ง" },
        { step: "brewing_drink", message: "ใส่เครื่องดื่ม" },
        { step: "completed", message: "เสร็จสิ้น" },
      ];

      for (const step of steps) {
        console.log(`🔄 ${step.step}: ${step.message}`);
        
        await fetch(`${BASE_URL}/api/hardware/progress`, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({
            orderId: data.order.orderId,
            hardwareId: "test-hardware",
            step: step.step,
            status: step.step === "completed" ? "completed" : "in_progress",
            message: step.message,
          }),
        });
        
        // Wait 2 seconds between steps
        await new Promise(resolve => setTimeout(resolve, 2000));
      }
      
      console.log(`✅ Order ${data.order.orderId} completed!`);
    } else {
      console.log("ℹ️ No orders available for hardware");
    }
  } catch (error) {
    console.error("❌ Error simulating hardware:", error.message);
  }
}

async function main() {
  console.log("🧪 Starting Queue Test");
  console.log(`Target: ${BASE_URL}`);
  
  // Create 3 test orders
  const orders = [];
  for (let i = 1; i <= 3; i++) {
    const order = await createTestOrder(i);
    if (order) orders.push(order);
    await new Promise(resolve => setTimeout(resolve, 1000)); // Wait 1 second between orders
  }
  
  console.log(`\n✅ Created ${orders.length} test orders`);
  
  // Check initial queue status
  await checkQueueStatus();
  
  // Process orders one by one
  for (let i = 0; i < orders.length; i++) {
    await simulateHardwareProcessing();
    await checkQueueStatus();
    
    if (i < orders.length - 1) {
      console.log("\n⏳ Waiting 3 seconds before next order...");
      await new Promise(resolve => setTimeout(resolve, 3000));
    }
  }
  
  console.log("\n🎉 Test completed!");
}

main().catch(console.error);

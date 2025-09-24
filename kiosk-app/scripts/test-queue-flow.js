#!/usr/bin/env node
/*
  Test script for queue flow - specifically testing position updates
  - Creates multiple orders
  - Simulates hardware processing
  - Checks queue position updates
*/

const fetch = global.fetch || (await import("node-fetch")).default;

const BASE_URL = process.env.BASE_URL || "http://localhost:3000";

async function createOrder(orderNum) {
  console.log(`\n=== Creating Order ${orderNum} ===`);
  
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

    // Create payment and mark as paid
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

    // Mock payment success
    await fetch(`${BASE_URL}/api/payments/mock-webhook`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ orderId: order.id, status: "PAID" }),
    });

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
    console.error(`❌ Error creating order ${orderNum}:`, error.message);
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
    return data.queue || [];
  } catch (error) {
    console.error("❌ Error checking queue:", error.message);
    return [];
  }
}

async function simulateHardwareProcessing(orderId) {
  console.log(`\n=== Simulating Hardware Processing for ${orderId.slice(-8)} ===`);
  
  try {
    // Get order from hardware
    const response = await fetch(`${BASE_URL}/api/hardware/orders?hardwareId=test-hardware`, {
      headers: { "X-API-Key": "dev-hardware-key" }
    });
    
    const data = await response.json();
    
    if (data.success && data.order) {
      console.log(`✅ Hardware got order: ${data.order.orderId}`);
      
      // Simulate brewing steps quickly
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
        
        // Wait 1 second between steps for faster testing
        await new Promise(resolve => setTimeout(resolve, 1000));
      }
      
      console.log(`✅ Order ${data.order.orderId.slice(-8)} completed!`);
      return true;
    } else {
      console.log("ℹ️ No orders available for hardware");
      return false;
    }
  } catch (error) {
    console.error("❌ Error simulating hardware:", error.message);
    return false;
  }
}

async function waitForQueueUpdate(expectedPositions) {
  console.log(`\n⏳ Waiting for queue positions to update...`);
  console.log(`Expected positions:`, expectedPositions);
  
  for (let i = 0; i < 10; i++) { // Try 10 times
    await new Promise(resolve => setTimeout(resolve, 1000));
    const queue = await checkQueueStatus();
    
    const currentPositions = queue.map(q => ({
      orderId: q.orderId.slice(-8),
      position: q.queuePosition,
      status: q.status
    }));
    
    console.log(`Current positions:`, currentPositions);
    
    // Check if positions match expected
    let matches = true;
    for (const expected of expectedPositions) {
      const found = queue.find(q => q.orderId.includes(expected.orderId.slice(-8)));
      if (!found || found.queuePosition !== expected.position) {
        matches = false;
        break;
      }
    }
    
    if (matches) {
      console.log(`✅ Queue positions updated correctly!`);
      return true;
    }
  }
  
  console.log(`❌ Queue positions did not update as expected`);
  return false;
}

async function main() {
  console.log("🧪 Starting Queue Flow Test");
  console.log(`Target: ${BASE_URL}`);
  
  // Create 3 test orders
  const orders = [];
  for (let i = 1; i <= 3; i++) {
    const order = await createOrder(i);
    if (order) orders.push(order);
    await new Promise(resolve => setTimeout(resolve, 500)); // Wait 0.5 second between orders
  }
  
  console.log(`\n✅ Created ${orders.length} test orders`);
  
  // Check initial queue status
  await checkQueueStatus();
  
  // Process first order
  console.log(`\n🎯 Processing first order (should be position 1)...`);
  await simulateHardwareProcessing(orders[0].orderId);
  
  // Wait for queue to reorder
  await waitForQueueUpdate([
    { orderId: orders[1].orderId, position: 1 },
    { orderId: orders[2].orderId, position: 2 }
  ]);
  
  // Check queue status after first order completion
  await checkQueueStatus();
  
  // Process second order
  console.log(`\n🎯 Processing second order (should now be position 1)...`);
  await simulateHardwareProcessing(orders[1].orderId);
  
  // Wait for queue to reorder
  await waitForQueueUpdate([
    { orderId: orders[2].orderId, position: 1 }
  ]);
  
  // Check final queue status
  await checkQueueStatus();
  
  console.log("\n🎉 Queue Flow Test completed!");
  console.log("\n📋 Summary:");
  console.log("- Created 3 orders");
  console.log("- Processed first order, second order should move to position 1");
  console.log("- Processed second order, third order should move to position 1");
  console.log("- Check browser console for redirect logs");
}

main().catch(console.error);

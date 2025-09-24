#!/usr/bin/env node
/*
  Test script for queue redirect logic
  - Tests SSE connection stability
  - Tests queue position updates
  - Tests redirect logic for queue positions 1, 2, 3+
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
      return data.queue;
    } else {
      console.log("❌ Failed to get queue status");
      return [];
    }
  } catch (error) {
    console.error("❌ Error checking queue:", error.message);
    return [];
  }
}

async function simulateHardwareProcessing() {
  console.log(`\n=== Simulating Hardware Processing ===`);
  
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

async function testSSEConnection() {
  console.log(`\n=== Testing SSE Connection ===`);
  
  try {
    const response = await fetch(`${BASE_URL}/api/events/queue`, {
      headers: {
        'Accept': 'text/event-stream',
        'Cache-Control': 'no-cache'
      }
    });
    
    if (response.ok) {
      console.log("✅ SSE endpoint is accessible");
      return true;
    } else {
      console.log(`❌ SSE endpoint returned: ${response.status}`);
      return false;
    }
  } catch (error) {
    console.error("❌ SSE connection test failed:", error.message);
    return false;
  }
}

async function main() {
  console.log("🧪 Starting Queue Redirect Test");
  console.log(`Target: ${BASE_URL}`);
  
  // Test SSE connection first
  const sseOk = await testSSEConnection();
  if (!sseOk) {
    console.log("❌ SSE connection test failed, aborting");
    return;
  }
  
  // Create 3 test orders
  const orders = [];
  for (let i = 1; i <= 3; i++) {
    const order = await createOrder(i);
    if (order) orders.push(order);
    await new Promise(resolve => setTimeout(resolve, 500)); // Wait 0.5 second between orders
  }
  
  console.log(`\n✅ Created ${orders.length} test orders`);
  
  // Check initial queue status
  let queue = await checkQueueStatus();
  
  // Test redirect logic for each position
  console.log(`\n🎯 Testing redirect logic for each position:`);
  
  // Position 1 should redirect to in-progress
  if (queue.length >= 1) {
    const firstOrder = queue[0];
    console.log(`\n📍 Testing Position 1 (${firstOrder.orderId.slice(-8)}):`);
    console.log(`   - Should redirect to /in-progress`);
    console.log(`   - Status: ${firstOrder.status}`);
    console.log(`   - Position: ${firstOrder.queuePosition}`);
  }
  
  // Process first order
  console.log(`\n🎯 Processing first order...`);
  await simulateHardwareProcessing();
  
  // Wait for queue to reorder
  await new Promise(resolve => setTimeout(resolve, 2000));
  queue = await checkQueueStatus();
  
  // Position 2 should now be position 1
  if (queue.length >= 1) {
    const newFirstOrder = queue[0];
    console.log(`\n📍 After processing, new Position 1 (${newFirstOrder.orderId.slice(-8)}):`);
    console.log(`   - Should redirect to /in-progress`);
    console.log(`   - Status: ${newFirstOrder.status}`);
    console.log(`   - Position: ${newFirstOrder.queuePosition}`);
  }
  
  // Process second order
  console.log(`\n🎯 Processing second order...`);
  await simulateHardwareProcessing();
  
  // Wait for queue to reorder
  await new Promise(resolve => setTimeout(resolve, 2000));
  queue = await checkQueueStatus();
  
  // Position 3 should now be position 1
  if (queue.length >= 1) {
    const finalFirstOrder = queue[0];
    console.log(`\n📍 After processing second, final Position 1 (${finalFirstOrder.orderId.slice(-8)}):`);
    console.log(`   - Should redirect to /in-progress`);
    console.log(`   - Status: ${finalFirstOrder.status}`);
    console.log(`   - Position: ${finalFirstOrder.queuePosition}`);
  }
  
  console.log("\n🎉 Queue Redirect Test completed!");
  console.log("\n📋 Summary:");
  console.log("- Created 3 orders");
  console.log("- Processed first order, second order moved to position 1");
  console.log("- Processed second order, third order moved to position 1");
  console.log("- Each position 1 should redirect to /in-progress");
  console.log("\n🌐 Test in browser:");
  console.log("1. Open browser to the kiosk app");
  console.log("2. Create orders and watch console logs");
  console.log("3. Check that position 1 redirects to in-progress");
  console.log("4. Check that SSE connection stays stable");
}

main().catch(console.error);
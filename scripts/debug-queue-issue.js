#!/usr/bin/env node
/*
  Debug script for queue sync issues
  - Tests queue consistency between multiple users
  - Tests SSE connection stability
  - Tests queue cleanup
*/

const fetch = global.fetch || (await import("node-fetch")).default;

const BASE_URL = process.env.BASE_URL || "http://localhost:3000";

async function checkQueueStatus() {
  console.log(`\n=== Checking Queue Status ===`);
  
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

async function clearQueue() {
  console.log(`\n=== Clearing Queue ===`);
  
  try {
    const response = await fetch(`${BASE_URL}/api/queue/clear`, {
      method: "POST"
    });
    const data = await response.json();
    
    if (data.success) {
      console.log(`✅ Cleared ${data.clearedOrders.length} orders from queue`);
      return true;
    } else {
      console.log("❌ Failed to clear queue");
      return false;
    }
  } catch (error) {
    console.error("❌ Error clearing queue:", error.message);
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
          toppings: []
        },
        amount: 50
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
        toppings: [],
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

async function simulateMultipleUsers() {
  console.log(`\n=== Simulating Multiple Users ===`);
  
  // Clear queue first
  await clearQueue();
  
  // Create 3 orders
  const orders = [];
  for (let i = 1; i <= 3; i++) {
    const order = await createTestOrder(i);
    if (order) orders.push(order);
    await new Promise(resolve => setTimeout(resolve, 1000)); // Wait 1 second between orders
  }
  
  console.log(`\n✅ Created ${orders.length} test orders`);
  
  // Check queue status multiple times
  for (let i = 1; i <= 3; i++) {
    console.log(`\n--- Check ${i} ---`);
    await checkQueueStatus();
    await new Promise(resolve => setTimeout(resolve, 2000)); // Wait 2 seconds between checks
  }
  
  return orders;
}

async function main() {
  console.log("🐛 Starting Queue Debug Test");
  console.log(`Target: ${BASE_URL}`);
  
  // Test SSE connection first
  const sseOk = await testSSEConnection();
  if (!sseOk) {
    console.log("❌ SSE connection test failed, aborting");
    return;
  }
  
  // Test queue consistency
  await simulateMultipleUsers();
  
  console.log("\n🎯 Debug Test completed!");
  console.log("\n📋 Next steps:");
  console.log("1. Check browser console for SSE connection logs");
  console.log("2. Check server logs for queue reordering");
  console.log("3. Test with multiple browser tabs");
  console.log("4. Check if positions sync correctly");
  console.log("\n🌐 Test URLs:");
  console.log("- Queue status: GET /api/queue");
  console.log("- Clear queue: POST /api/queue/clear");
  console.log("- SSE endpoint: /api/events/queue");
}

main().catch(console.error);

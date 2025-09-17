import { NextRequest, NextResponse } from "next/server";
import { queueService } from "@/lib/queue-service";

// GET /api/hardware/test-brewing - Trigger ESP32 brewing simulation for testing
export async function GET(request: NextRequest) {
  const hardwareId =
    request.nextUrl.searchParams.get("hardwareId") || "esp32-001";

  try {
    // Create a test order for ESP32 to process
    const testOrderId = `test_${Date.now()}`;
    const testOrder = await queueService.addToQueue({
      orderId: testOrderId,
      status: "pending",
      createdAt: new Date(),
      customer: {
        sessionId: "test-session",
        deviceId: "test-device",
      },
      order: {
        drinkName: "ชาไทย (ทดสอบ)",
        toppings: ["ไข่มุก", "วิปครีม"],
        totalAmount: 50,
        size: "Regular",
      },
    });

    // Immediately assign to hardware for processing
    await queueService.updateOrderStatus(
      testOrder.orderId,
      "preparing",
      hardwareId
    );

    return NextResponse.json({
      success: true,
      message: "Test brewing started",
      testOrder: {
        orderId: testOrder.orderId,
        drinkName: testOrder.order.drinkName,
        toppings: testOrder.order.toppings,
        status: "preparing",
      },
      instructions: [
        "1. ESP32 should start brewing simulation immediately",
        "2. Check ESP32 Serial Monitor for brewing progress",
        "3. LEDs should change every 20 seconds",
        "4. Visit /in-progress page to see web updates",
        "5. Total process takes 100 seconds (5×20s)",
      ],
      estimatedCompletionTime: "100 seconds",
    });
  } catch (error) {
    console.error("Error starting test brewing:", error);
    return NextResponse.json(
      {
        error: "Failed to start test brewing",
        details: String(error),
      },
      { status: 500 }
    );
  }
}

// POST /api/hardware/test-brewing - Force ESP32 to start with specific order
export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const {
      orderId,
      drinkName,
      toppings = [],
      hardwareId = "esp32-001",
    } = body;

    if (!orderId || !drinkName) {
      return NextResponse.json(
        { error: "orderId and drinkName are required" },
        { status: 400 }
      );
    }

    // Create test order with provided details
    const testOrder = await queueService.addToQueue({
      orderId,
      status: "pending",
      createdAt: new Date(),
      customer: {
        sessionId: "manual-test-session",
        deviceId: "manual-test-device",
      },
      order: {
        drinkName,
        toppings,
        totalAmount: 50,
        size: "Regular",
      },
    });

    // Assign to hardware immediately
    await queueService.updateOrderStatus(orderId, "preparing", hardwareId);

    return NextResponse.json({
      success: true,
      message: `Manual test brewing started for ${drinkName}`,
      testOrder: {
        orderId,
        drinkName,
        toppings,
        status: "preparing",
        hardwareId,
      },
      webUrl: `/in-progress?orderId=${orderId}`,
      esp32Instructions: [
        "ESP32 will detect this order in next polling cycle (≤5 seconds)",
        "Brewing process will start automatically",
        "Monitor Serial output for detailed progress",
      ],
    });
  } catch (error) {
    console.error("Error starting manual test brewing:", error);
    return NextResponse.json(
      {
        error: "Failed to start manual test brewing",
        details: String(error),
      },
      { status: 500 }
    );
  }
}

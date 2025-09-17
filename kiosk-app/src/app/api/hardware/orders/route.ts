import { NextRequest, NextResponse } from "next/server";
import { queueService } from "@/lib/queue-service";
import { z } from "zod";

// Hardware authentication middleware
const HARDWARE_API_KEY = process.env.HARDWARE_API_KEY || "dev-hardware-key";

function authenticateHardware(request: NextRequest): string | null {
  const authHeader = request.headers.get("authorization");
  const apiKey = request.headers.get("x-api-key");

  if (
    authHeader?.startsWith("Bearer ") &&
    authHeader.split(" ")[1] === HARDWARE_API_KEY
  ) {
    return authHeader.split(" ")[1];
  }

  if (apiKey === HARDWARE_API_KEY) {
    return apiKey;
  }

  return null;
}

// GET /api/hardware/orders - Poll for new orders
export async function GET(request: NextRequest) {
  const auth = authenticateHardware(request);
  if (!auth) {
    return NextResponse.json({ error: "Unauthorized" }, { status: 401 });
  }

  const hardwareId =
    request.nextUrl.searchParams.get("hardwareId") || "esp32-001";

  try {
    // Update heartbeat first
    await queueService.updateHardwareStatus(hardwareId, "idle");

    // Get next pending order
    const nextOrder = await queueService.getNextOrder();

    if (nextOrder) {
      // Mark as preparing and assign to hardware
      await queueService.updateOrderStatus(
        nextOrder.orderId, // Use orderId instead of id
        "preparing",
        hardwareId
      );
      await queueService.updateHardwareStatus(
        hardwareId,
        "busy",
        nextOrder.orderId
      );

      return NextResponse.json({
        success: true,
        order: {
          id: nextOrder.orderId, // Use orderId consistently
          orderId: nextOrder.orderId,
          drinkName: nextOrder.order.drinkName,
          toppings: nextOrder.order.toppings,
          size: nextOrder.order.size,
          queuePosition: nextOrder.queuePosition,
        },
        message: "New order assigned to hardware",
      });
    }

    // No orders - hardware stays idle
    return NextResponse.json({
      success: true,
      order: null,
      message: "No pending orders",
    });
  } catch (error) {
    console.error("Hardware order polling error:", error);
    return NextResponse.json(
      { error: "Internal server error" },
      { status: 500 }
    );
  }
}

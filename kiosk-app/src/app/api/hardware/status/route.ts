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

// Status update schema
const StatusUpdateSchema = z.object({
  orderId: z.string(),
  status: z.enum(["preparing", "brewing", "completed"]),
  step: z
    .enum([
      "preparing_cup",
      "adding_toppings",
      "adding_ice",
      "brewing_drink",
      "completed",
    ])
    .optional(),
  message: z.string().optional(),
  hardwareId: z.string(),
  error: z.boolean().optional(),
});

// POST /api/hardware/status - Update order status
export async function POST(request: NextRequest) {
  const auth = authenticateHardware(request);
  if (!auth) {
    return NextResponse.json({ error: "Unauthorized" }, { status: 401 });
  }

  try {
    const body = await request.json();
    const updateData = StatusUpdateSchema.parse(body);

    // Update order status
    await queueService.updateOrderStatus(
      updateData.orderId,
      updateData.status,
      updateData.hardwareId
    );

    // Add brewing step if provided
    if (updateData.step) {
      await queueService.addBrewingStep({
        orderId: updateData.orderId,
        step: updateData.step,
        status: updateData.error ? "error" : "completed",
        timestamp: new Date(),
        message: updateData.message,
        hardwareId: updateData.hardwareId,
      });
    }

    // Update hardware status
    if (updateData.status === "completed") {
      await queueService.updateHardwareStatus(updateData.hardwareId, "idle");
    } else {
      await queueService.updateHardwareStatus(
        updateData.hardwareId,
        "busy",
        updateData.orderId
      );
    }

    console.log(
      `Hardware ${updateData.hardwareId} updated order ${updateData.orderId} to ${updateData.status}`
    );

    return NextResponse.json({
      success: true,
      message: "Status updated successfully",
    });
  } catch (error) {
    console.error("Hardware status update error:", error);
    return NextResponse.json(
      {
        error: "Invalid request data",
        details: error instanceof z.ZodError ? error.issues : String(error),
      },
      { status: 400 }
    );
  }
}

// GET /api/hardware/status - Get hardware status
export async function GET(request: NextRequest) {
  const auth = authenticateHardware(request);
  if (!auth) {
    return NextResponse.json({ error: "Unauthorized" }, { status: 401 });
  }

  const hardwareId =
    request.nextUrl.searchParams.get("hardwareId") || "esp32-001";

  try {
    // Send heartbeat
    await queueService.updateHardwareStatus(hardwareId, "idle");

    const queue = await queueService.getQueueStatus();

    return NextResponse.json({
      success: true,
      hardwareId,
      queueLength: queue.length,
      estimatedWaitTime: queue.length * 3, // 3 minutes per order
      timestamp: new Date().toISOString(),
    });
  } catch (error) {
    console.error("Hardware status check error:", error);
    return NextResponse.json(
      { error: "Internal server error" },
      { status: 500 }
    );
  }
}

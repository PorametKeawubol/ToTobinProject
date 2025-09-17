import { NextRequest, NextResponse } from "next/server";
import { queueService } from "@/lib/queue-service";
import { z } from "zod";

// Hardware authentication middleware
const HARDWARE_API_KEY = process.env.HARDWARE_API_KEY || "dev-hardware-key";

function authenticateHardware(request: NextRequest): string | null {
  const authHeader = request.headers.get("authorization");
  const apiKey = request.headers.get("x-api-key"); // lowercase
  const apiKeyUpper = request.headers.get("X-API-Key"); // uppercase for ESP32

  if (
    authHeader?.startsWith("Bearer ") &&
    authHeader.split(" ")[1] === HARDWARE_API_KEY
  ) {
    return authHeader.split(" ")[1];
  }

  // Check both lowercase and uppercase API key headers
  if (apiKey === HARDWARE_API_KEY || apiKeyUpper === HARDWARE_API_KEY) {
    return apiKey || apiKeyUpper;
  }

  return null;
}

// Status update schema
const StatusUpdateSchema = z.object({
  orderId: z.string(),
  status: z.enum(["pending", "preparing", "brewing", "completed", "cancelled"]),
  step: z
    .enum([
      "preparing_cup",
      "adding_toppings",
      "adding_ice",
      "brewing_drink",
      "completed",
    ])
    .optional(),
  ledState: z
    .object({
      preparing: z.boolean(),
      toppings: z.boolean(),
      ice: z.boolean(),
      brewing: z.boolean(),
      completed: z.boolean(),
    })
    .optional(),
  progress: z.number().min(0).max(100).optional(),
  message: z.string().optional(),
  hardwareId: z.string(),
  error: z.boolean().optional(),
  elapsedTime: z.number().optional(),
  totalTime: z.number().optional(),
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

    // Update hardware status based on order status
    if (updateData.status === "completed") {
      await queueService.updateHardwareStatus(updateData.hardwareId, "idle");
    } else if (
      updateData.status === "preparing" ||
      updateData.status === "brewing"
    ) {
      await queueService.updateHardwareStatus(
        updateData.hardwareId,
        "busy",
        updateData.orderId
      );
    }

    // Store LED state and progress information for web display
    const statusResponse = {
      success: true,
      message: "Status updated successfully",
      hardwareStatus: {
        orderId: updateData.orderId,
        status: updateData.status,
        step: updateData.step,
        ledState: updateData.ledState,
        progress: updateData.progress,
        elapsedTime: updateData.elapsedTime,
        totalTime: updateData.totalTime,
        timestamp: new Date().toISOString(),
      },
    };

    console.log(
      `Hardware ${updateData.hardwareId} updated order ${updateData.orderId}:`,
      {
        status: updateData.status,
        step: updateData.step,
        ledState: updateData.ledState,
        progress: updateData.progress,
      }
    );

    return NextResponse.json(statusResponse);
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
  const orderId = request.nextUrl.searchParams.get("orderId");

  try {
    // Send heartbeat
    await queueService.updateHardwareStatus(hardwareId, "idle");

    const queue = await queueService.getQueueStatus();

    // If orderId is specified, get specific order status
    if (orderId) {
      const order = queue.find((o) => o.orderId === orderId);
      if (order) {
        return NextResponse.json({
          success: true,
          order: {
            orderId: order.orderId,
            status: order.status,
            queuePosition: order.queuePosition,
            estimatedTime: order.estimatedTime,
            startedAt: order.startedAt,
            completedAt: order.completedAt,
          },
          hardwareId,
          timestamp: new Date().toISOString(),
        });
      } else {
        return NextResponse.json(
          {
            success: false,
            error: "Order not found",
            orderId,
          },
          { status: 404 }
        );
      }
    }

    // General hardware status
    return NextResponse.json({
      success: true,
      hardwareId,
      queueLength: queue.length,
      estimatedWaitTime: queue.length * 3, // 3 minutes per order
      currentOrders: queue.filter(
        (o) => o.status === "preparing" || o.status === "brewing"
      ),
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

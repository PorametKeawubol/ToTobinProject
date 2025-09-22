import { NextRequest, NextResponse } from "next/server";
import { queueService } from "@/lib/queue-service";
import { BrewingStepSchema } from "@/lib/queue-schemas";

// POST /api/hardware/progress - Hardware reports brewing step progress
export async function POST(request: NextRequest) {
  try {
    const body = await request.json();

    // Validate and normalize payload
    const parsed = BrewingStepSchema.parse({
      orderId: body.orderId,
      step: body.step,
      status: body.status,
      timestamp: body.timestamp ? new Date(body.timestamp) : new Date(),
      message: body.message,
      hardwareId: body.hardwareId || "esp32-001",
    });

    // Record step
    await queueService.addBrewingStep(parsed);

    // Update order status based on step
    if (parsed.step === "preparing_cup") {
      await queueService.updateOrderStatus(parsed.orderId, "preparing", parsed.hardwareId);
    } else if (
      parsed.step === "adding_toppings" ||
      parsed.step === "adding_ice" ||
      parsed.step === "brewing_drink"
    ) {
      await queueService.updateOrderStatus(parsed.orderId, "brewing", parsed.hardwareId);
    } else if (parsed.step === "completed" && parsed.status === "completed") {
      await queueService.updateOrderStatus(parsed.orderId, "completed", parsed.hardwareId);
    }

    return NextResponse.json({ success: true });
  } catch (error) {
    console.error("Hardware progress error:", error);
    return NextResponse.json(
      { error: "Failed to record hardware progress", details: String(error) },
      { status: 400 }
    );
  }
}



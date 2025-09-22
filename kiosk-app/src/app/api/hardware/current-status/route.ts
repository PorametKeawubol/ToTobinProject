import { NextRequest, NextResponse } from "next/server";
import { queueService } from "@/lib/queue-service";

// GET /api/hardware/current-status - Get current brewing status for web display
export async function GET(request: NextRequest) {
  const orderId = request.nextUrl.searchParams.get("orderId");

  if (!orderId) {
    return NextResponse.json(
      { error: "orderId parameter is required" },
      { status: 400 }
    );
  }

  try {
    // Get order by orderId (includes completed)
    const order = await queueService.getOrderByOrderId(orderId);

    if (!order) {
      return NextResponse.json({ error: "Order not found" }, { status: 404 });
    }

    // Get brewing steps for this order
    const brewingSteps = await queueService.getBrewingSteps(orderId);

    // Calculate progress based on steps completed
    let progress = 0;
    let currentStep = "waiting";
    let message = "รอเครื่องทำเครื่องดื่ม";

    if (order.status === "preparing" || order.status === "brewing") {
      // Count unique steps seen to reflect simulator events (in_progress or completed)
      const seenSteps = new Set(brewingSteps.map((s) => s.step));
      const totalSteps = 5; // preparing_cup, adding_toppings, adding_ice, brewing_drink, completed
      const completedCount = Math.min(totalSteps, seenSteps.size);
      progress = Math.round((completedCount / totalSteps) * 100);

      // Get current step from latest brewing step
      const latestStep = brewingSteps[brewingSteps.length - 1];
      if (latestStep) {
        currentStep = latestStep.step;
        message = latestStep.message || "กำลังดำเนินการ";
      }
    } else if (order.status === "completed") {
      progress = 100;
      currentStep = "completed";
      message = "เสร็จสิ้น! กรุณารับเครื่องดื่ม";
    } else if (order.status === "pending") {
      progress = 0;
      currentStep = "waiting";
      message = `อยู่ในคิวลำดับที่ ${order.queuePosition}`;
    }

    // Calculate estimated remaining time
    let estimatedTime = 0;
    if (order.status === "preparing" || order.status === "brewing") {
      const seenSteps = new Set(brewingSteps.map((s) => s.step));
      const remainingSteps = Math.max(0, 5 - seenSteps.size);
      estimatedTime = remainingSteps * 20; // 20 seconds per step
    } else if (order.status === "pending") {
      estimatedTime = order.estimatedTime * 60; // Convert minutes to seconds
    }

    // Build ledState for UI based on latest step
    const ledState = {
      preparing: currentStep === "preparing_cup",
      toppings: currentStep === "adding_toppings",
      ice: currentStep === "adding_ice",
      brewing: currentStep === "brewing_drink",
      completed: currentStep === "completed",
    };

    return NextResponse.json({
      success: true,
      order: {
        orderId: order.orderId,
        status: order.status,
        queuePosition: order.queuePosition,
        progress,
        currentStep,
        message,
        estimatedTime,
        drinkName: order.order.drinkName,
        toppings: order.order.toppings,
        totalAmount: order.order.totalAmount,
        ledState,
      },
      brewingSteps: brewingSteps.map((step) => ({
        step: step.step,
        status: step.status,
        timestamp: step.timestamp,
        message: step.message,
      })),
      timestamp: new Date().toISOString(),
    });
  } catch (error) {
    console.error("Error getting current status:", error);
    return NextResponse.json(
      { error: "Internal server error" },
      { status: 500 }
    );
  }
}

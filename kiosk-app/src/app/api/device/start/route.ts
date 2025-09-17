import { NextRequest, NextResponse } from "next/server";
import { deviceController } from "@/lib/device";
import { Orders } from "@/lib/orders";
import { z } from "zod";

const StartDeviceRequest = z.object({
  orderId: z.string(),
});

export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const { orderId } = StartDeviceRequest.parse(body);

    // Check if order exists and is in PAID status
    const order = Orders.get(orderId);
    if (!order) {
      return NextResponse.json({ error: "Order not found" }, { status: 404 });
    }

    if (order.status !== "PAID") {
      return NextResponse.json(
        { error: `Order status is ${order.status}, expected PAID` },
        { status: 400 }
      );
    }

    // Start the device
    const result = await deviceController.start(orderId);

    if (!result.success) {
      return NextResponse.json({ error: result.message }, { status: 409 });
    }

    // Update order status to DISPENSING
    Orders.updateStatus(orderId, "DISPENSING");

    // Simulate completion after brewing time (this would be handled by device callback in real implementation)
    setTimeout(() => {
      // Randomly simulate success or failure
      const success = Math.random() > 0.1; // 90% success rate
      if (success) {
        Orders.updateStatus(orderId, "DONE");
      } else {
        Orders.updateStatus(orderId, "ERROR");
      }
    }, Math.random() * (30000 - 15000) + 15000); // 15-30 seconds

    return NextResponse.json(result);
  } catch (error) {
    console.error("Error starting device:", error);
    return NextResponse.json(
      { error: "Failed to start device" },
      { status: 500 }
    );
  }
}

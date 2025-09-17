import { NextRequest, NextResponse } from "next/server";
import { Payments } from "@/lib/payments";
import { Orders } from "@/lib/orders";
import { z } from "zod";

const WebhookPayload = z.object({
  orderId: z.string(),
  status: z.enum(["PAID", "EXPIRED", "CANCELLED"]).optional().default("PAID"),
});

export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const { orderId, status } = WebhookPayload.parse(body);

    // Update payment status
    if (status === "PAID") {
      await Payments.markAsPaid(orderId);
    } else if (status === "EXPIRED") {
      await Payments.markAsExpired(orderId);
    } else if (status === "CANCELLED") {
      await Payments.markAsCancelled(orderId);
    }

    // Update order status
    if (status === "PAID") {
      Orders.updateStatus(orderId, "PAID");
    } else if (status === "EXPIRED" || status === "CANCELLED") {
      Orders.updateStatus(orderId, "CANCELLED");
    }

    return NextResponse.json({
      success: true,
      message: `Payment status updated to ${status} for order ${orderId}`,
    });
  } catch (error) {
    console.error("Error processing payment webhook:", error);
    return NextResponse.json(
      { error: "Failed to process payment webhook" },
      { status: 500 }
    );
  }
}

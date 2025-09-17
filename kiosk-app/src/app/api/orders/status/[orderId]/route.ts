import { NextRequest, NextResponse } from "next/server";
import { Orders } from "@/lib/orders";

export async function GET(
  request: NextRequest,
  { params }: { params: Promise<{ orderId: string }> }
) {
  try {
    const { orderId } = await params;
    const order = Orders.get(orderId);

    if (!order) {
      return NextResponse.json({ error: "Order not found" }, { status: 404 });
    }

    return NextResponse.json(order);
  } catch (error) {
    console.error("Error getting order status:", error);
    return NextResponse.json(
      { error: "Failed to get order status" },
      { status: 500 }
    );
  }
}

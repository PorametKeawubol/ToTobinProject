import { NextRequest, NextResponse } from "next/server";
import { Payments } from "@/lib/payments";

export async function GET(
  request: NextRequest,
  { params }: { params: Promise<{ orderId: string }> }
) {
  try {
    const { orderId } = await params;
    const status = await Payments.status(orderId);

    return NextResponse.json({ orderId, status });
  } catch (error) {
    console.error("Error getting payment status:", error);
    return NextResponse.json(
      { error: "Failed to get payment status" },
      { status: 500 }
    );
  }
}

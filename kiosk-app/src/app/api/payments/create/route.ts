import { NextRequest, NextResponse } from "next/server";
import { Payments } from "@/lib/payments";
import { PaymentRequest } from "@/lib/schemas";

export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const { orderId, amount } = PaymentRequest.parse(body);

    const paymentResult = await Payments.create(orderId, amount);

    return NextResponse.json(paymentResult);
  } catch (error) {
    console.error("Error creating payment:", error);
    return NextResponse.json(
      { error: "Failed to create payment" },
      { status: 500 }
    );
  }
}

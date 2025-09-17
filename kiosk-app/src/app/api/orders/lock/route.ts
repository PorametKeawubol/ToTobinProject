import { NextRequest, NextResponse } from "next/server";
import { kioskLock } from "@/lib/kioskLock";
import { z } from "zod";

const LockRequest = z.object({
  orderId: z.string(),
});

export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const { orderId } = LockRequest.parse(body);

    const result = kioskLock.lock(orderId);

    if (!result.success) {
      return NextResponse.json(
        { error: result.message },
        { status: 409 } // Conflict
      );
    }

    return NextResponse.json(result);
  } catch (error) {
    console.error("Error locking kiosk:", error);
    return NextResponse.json(
      { error: "Failed to lock kiosk" },
      { status: 500 }
    );
  }
}

export async function DELETE() {
  try {
    const result = kioskLock.unlock();
    return NextResponse.json(result);
  } catch (error) {
    console.error("Error unlocking kiosk:", error);
    return NextResponse.json(
      { error: "Failed to unlock kiosk" },
      { status: 500 }
    );
  }
}

export async function GET() {
  try {
    const status = kioskLock.getStatus();
    return NextResponse.json(status);
  } catch (error) {
    console.error("Error getting kiosk lock status:", error);
    return NextResponse.json(
      { error: "Failed to get kiosk lock status" },
      { status: 500 }
    );
  }
}

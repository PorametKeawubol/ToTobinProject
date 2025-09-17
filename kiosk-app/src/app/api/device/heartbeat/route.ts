import { NextRequest, NextResponse } from "next/server";
import { deviceController } from "@/lib/device";

export async function GET() {
  try {
    const heartbeat = await deviceController.heartbeat();
    return NextResponse.json(heartbeat);
  } catch (error) {
    console.error("Error getting device heartbeat:", error);
    return NextResponse.json(
      { error: "Failed to get device heartbeat" },
      { status: 500 }
    );
  }
}

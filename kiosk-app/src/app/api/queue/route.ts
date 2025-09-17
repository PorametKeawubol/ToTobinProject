import { NextRequest, NextResponse } from "next/server";
import { queueService, useQueueStore } from "@/lib/queue-service";

// GET /api/queue - Get current queue status
export async function GET(request: NextRequest) {
  try {
    const queue = await queueService.getQueueStatus();

    return NextResponse.json({
      success: true,
      queue: queue.map((order) => ({
        id: order.id,
        queuePosition: order.queuePosition,
        status: order.status,
        estimatedTime: order.estimatedTime,
        drinkName: order.order.drinkName,
        createdAt: order.createdAt,
      })),
      totalInQueue: queue.length,
    });
  } catch (error) {
    console.error("Queue status error:", error);
    return NextResponse.json(
      { error: "Internal server error" },
      { status: 500 }
    );
  }
}

// POST /api/queue - Add order to queue
export async function POST(request: NextRequest) {
  try {
    const body = await request.json();

    // Add order to queue
    const queueOrder = await queueService.addToQueue({
      orderId: body.orderId,
      status: "pending",
      createdAt: new Date(),
      customer: {
        sessionId: body.sessionId || "anonymous",
      },
      order: {
        drinkName: body.drinkName,
        toppings: body.toppings || [],
        totalAmount: body.totalAmount,
        size: body.size || "Regular",
      },
    });

    return NextResponse.json({
      success: true,
      queueOrder: {
        id: queueOrder.id,
        queuePosition: queueOrder.queuePosition,
        estimatedTime: queueOrder.estimatedTime,
        status: queueOrder.status,
      },
    });
  } catch (error) {
    console.error("Add to queue error:", error);
    return NextResponse.json(
      { error: "Internal server error" },
      { status: 500 }
    );
  }
}

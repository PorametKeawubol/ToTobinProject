import { NextRequest, NextResponse } from "next/server";
import { queueService } from "@/lib/queue-service";

// POST /api/queue/clear - Clear all orders from queue (for debugging)
export async function POST(request: NextRequest) {
  try {
    // Get current queue
    const queue = await queueService.getQueueStatus();
    
    // Clear all orders
    await queueService.clearAllOrders();
    
    console.log(`Cleared ${queue.length} orders from queue`);
    
    return NextResponse.json({
      success: true,
      message: `Cleared ${queue.length} orders from queue`,
      clearedOrders: queue.map(q => ({
        id: q.id,
        orderId: q.orderId,
        status: q.status,
        queuePosition: q.queuePosition
      }))
    });
  } catch (error) {
    console.error("Error clearing queue:", error);
    return NextResponse.json(
      { error: "Failed to clear queue" },
      { status: 500 }
    );
  }
}

// GET /api/queue/clear - Get queue status before clearing
export async function GET() {
  try {
    const queue = await queueService.getQueueStatus();
    
    return NextResponse.json({
      success: true,
      queueLength: queue.length,
      orders: queue.map(q => ({
        id: q.id,
        orderId: q.orderId,
        status: q.status,
        queuePosition: q.queuePosition,
        createdAt: q.createdAt,
        hardwareId: q.hardwareId
      }))
    });
  } catch (error) {
    console.error("Error getting queue status:", error);
    return NextResponse.json(
      { error: "Failed to get queue status" },
      { status: 500 }
    );
  }
}

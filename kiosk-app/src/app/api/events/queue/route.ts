import { NextRequest } from "next/server";
import { queueService } from "@/lib/queue-service";

// Server-Sent Events for real-time updates
export async function GET(request: NextRequest) {
  const encoder = new TextEncoder();

  // Create a readable stream
  const stream = new ReadableStream({
    start(controller) {
      console.log("SSE connection established");

      // Send initial queue status
      const sendQueueUpdate = async () => {
        try {
          const queue = await queueService.getQueueStatus();
          console.log(`SSE sending queue update: ${queue.length} orders`, {
            timestamp: new Date().toISOString(),
            orders: queue.map(q => ({
              id: q.id,
              orderId: q.orderId,
              status: q.status,
              queuePosition: q.queuePosition,
              hardwareId: q.hardwareId,
              createdAt: q.createdAt.toISOString()
            }))
          });
          
          const data = JSON.stringify({
            type: "queue-update",
            data: {
              queue: queue.map((order) => ({
                ...order,
                // Ensure all necessary fields are included
                id: order.id,
                orderId: order.orderId,
                queuePosition: order.queuePosition,
                status: order.status,
                estimatedTime: order.estimatedTime,
                createdAt: order.createdAt,
                order: order.order,
                customer: order.customer,
                startedAt: order.startedAt,
                completedAt: order.completedAt,
                hardwareId: order.hardwareId,
              })),
              totalInQueue: queue.length,
              timestamp: new Date().toISOString(),
            },
          });

          controller.enqueue(encoder.encode(`data: ${data}\n\n`));
        } catch (error) {
          console.error("SSE queue update error:", error);
        }
      };

      // Send initial data
      sendQueueUpdate();

      // Setup periodic updates
      const interval = setInterval(sendQueueUpdate, 1000); // Update every 1 second for faster updates

      // Heartbeat to keep connection alive
      const heartbeat = setInterval(() => {
        controller.enqueue(encoder.encode(": heartbeat\n\n"));
      }, 30000); // Every 30 seconds

      // Cleanup on close
      request.signal.addEventListener("abort", () => {
        clearInterval(interval);
        clearInterval(heartbeat);
        controller.close();
        console.log("SSE connection closed");
      });
    },
  });

  // Return SSE response
  return new Response(stream, {
    headers: {
      "Content-Type": "text/event-stream",
      "Cache-Control": "no-cache",
      Connection: "keep-alive",
      "Access-Control-Allow-Origin": "*",
      "Access-Control-Allow-Headers": "Content-Type",
    },
  });
}

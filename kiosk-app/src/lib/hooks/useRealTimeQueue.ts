import { useEffect, useState } from "react";
import { OrderQueue } from "@/lib/queue-schemas";

interface QueueUpdateData {
  queue: OrderQueue[];
  totalInQueue: number;
  timestamp: string;
}

export function useRealTimeQueue() {
  const [queueData, setQueueData] = useState<QueueUpdateData | null>(null);
  const [isConnected, setIsConnected] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    let eventSource: EventSource | null = null;

    const connectSSE = () => {
      try {
        eventSource = new EventSource("/api/events/queue");

        eventSource.onopen = () => {
          console.log("SSE connected");
          setIsConnected(true);
          setError(null);
        };

        eventSource.onmessage = (event) => {
          try {
            const eventData = JSON.parse(event.data);

            if (eventData.type === "queue-update") {
              setQueueData(eventData.data);
            }
          } catch (err) {
            console.error("SSE message parse error:", err);
          }
        };

        eventSource.onerror = (event) => {
          console.error("SSE error:", event);
          setIsConnected(false);
          setError("Connection lost. Retrying...");

          // Auto-reconnect after 3 seconds
          setTimeout(() => {
            if (eventSource?.readyState === EventSource.CLOSED) {
              connectSSE();
            }
          }, 3000);
        };
      } catch (err) {
        console.error("SSE connection error:", err);
        setError("Failed to connect");
      }
    };

    connectSSE();

    // Cleanup
    return () => {
      if (eventSource) {
        eventSource.close();
        setIsConnected(false);
      }
    };
  }, []);

  return {
    queueData,
    isConnected,
    error,
  };
}

// Hook สำหรับติดตาม order เฉพาะ
export function useOrderStatus(orderId: string | null) {
  const { queueData } = useRealTimeQueue();
  const [orderStatus, setOrderStatus] = useState<OrderQueue | null>(null);

  useEffect(() => {
    if (orderId && queueData) {
      const order = queueData.queue.find((q) => q.id === orderId);
      setOrderStatus(order || null);
    }
  }, [orderId, queueData]);

  return orderStatus;
}

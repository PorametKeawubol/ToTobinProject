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
    let pollInterval: ReturnType<typeof setInterval> | null = null;
    let reconnectTimeout: ReturnType<typeof setTimeout> | null = null;

    const connectSSE = () => {
      try {
        // Clean up existing connection
        if (eventSource) {
          eventSource.close();
        }
        if (reconnectTimeout) {
          clearTimeout(reconnectTimeout);
        }

        eventSource = new EventSource("/api/events/queue");

        eventSource.onopen = () => {
          console.log("SSE connected successfully");
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
          
          // Only show error if connection is actually lost
          if (eventSource?.readyState === EventSource.CLOSED) {
            setIsConnected(false);
            setError("Connection lost. Retrying...");

            // Clear existing intervals
            if (pollInterval) {
              clearInterval(pollInterval);
              pollInterval = null;
            }

            // Auto-reconnect after 1 second (even faster)
            reconnectTimeout = setTimeout(() => {
              if (eventSource?.readyState === EventSource.CLOSED) {
                console.log("Attempting SSE reconnection...");
                connectSSE();
              }
            }, 1000);

            // Fallback polling while disconnected
            if (!pollInterval) {
              pollInterval = setInterval(async () => {
                try {
                  const res = await fetch("/api/queue");
                  const data = await res.json();
                  if (data?.success) {
                    setQueueData({
                      queue: data.queue,
                      totalInQueue: data.totalInQueue,
                      timestamp: new Date().toISOString(),
                    });
                    // Clear error if polling works
                    setError(null);
                  }
                } catch (e) {
                  console.error("Polling error:", e);
                }
              }, 1500); // Faster polling
            }
          } else if (eventSource?.readyState === EventSource.CONNECTING) {
            // Connection is trying to reconnect
            console.log("SSE reconnecting...");
            setError("Reconnecting...");
          } else {
            // For other errors, just log but don't show error to user
            console.warn("SSE warning:", event);
          }
        };
      } catch (err) {
        console.error("SSE connection error:", err);
        setError("Failed to connect");
        // Start fallback polling immediately
        pollInterval = setInterval(async () => {
          try {
            const res = await fetch("/api/queue");
            const data = await res.json();
            if (data?.success) {
              setQueueData({
                queue: data.queue,
                totalInQueue: data.totalInQueue,
                timestamp: new Date().toISOString(),
              });
            }
          } catch (_e) {}
        }, 2000);
      }
    };

    connectSSE();

    // Cleanup
    return () => {
      if (eventSource) {
        eventSource.close();
        setIsConnected(false);
      }
      if (pollInterval) {
        clearInterval(pollInterval);
        pollInterval = null;
      }
      if (reconnectTimeout) {
        clearTimeout(reconnectTimeout);
        reconnectTimeout = null;
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
      console.log(`useOrderStatus: Looking for orderId=${orderId} in queue:`, {
        totalOrders: queueData.queue.length,
        orders: queueData.queue.map(q => ({ 
          id: q.id, 
          orderId: q.orderId,
          status: q.status, 
          position: q.queuePosition 
        })),
        lookingFor: orderId
      });
      
      // Search by orderId first, then by id as fallback
      const order = queueData.queue.find((q) => q.orderId === orderId || q.id === orderId);
      
      if (order) {
        console.log(`useOrderStatus: Found order:`, { 
          id: order.id, 
          orderId: order.orderId,
          status: order.status, 
          position: order.queuePosition 
        });
        setOrderStatus(order);
      } else {
        console.log(`useOrderStatus: No order found for orderId=${orderId}. Available orders:`, 
          queueData.queue.map(q => ({ id: q.id, orderId: q.orderId, status: q.status }))
        );
        setOrderStatus(null);
      }
    } else if (orderId && !queueData) {
      console.log(`useOrderStatus: orderId=${orderId} but no queueData yet`);
    } else if (!orderId) {
      console.log(`useOrderStatus: no orderId provided`);
    }
  }, [orderId, queueData]);

  return orderStatus;
}

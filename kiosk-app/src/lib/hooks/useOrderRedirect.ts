import { useEffect } from "react";
import { useRouter } from "next/navigation";
import { useQueueStore } from "@/lib/queue-service";

/**
 * Hook to handle automatic redirection based on order status
 * คนที่สั่งแล้วเครื่องกำลังทำ → หน้า in-progress
 * คนอื่น → หน้า queue
 */
export function useOrderRedirect() {
  const router = useRouter();
  const { currentUserOrder } = useQueueStore();

  useEffect(() => {
    if (!currentUserOrder) {
      return;
    }

    // Check if this user's order is currently being processed
    const isCurrentOrderBeingProcessed =
      currentUserOrder.status === "preparing" ||
      currentUserOrder.status === "brewing";

    // Check if this is the first in queue (position 1) or being processed
    const isFirstInQueue = currentUserOrder.queuePosition === 1;
    
    // If this user's order is being processed OR is first in queue, go to in-progress
    if (isCurrentOrderBeingProcessed || (isFirstInQueue && currentUserOrder.status === "pending")) {
      console.log(`Redirecting to in-progress: status=${currentUserOrder.status}, position=${currentUserOrder.queuePosition}`);
      router.push("/in-progress");
      return;
    }

    // If completed, redirect to done page
    if (currentUserOrder.status === "completed") {
      router.push("/done");
      return;
    }

    // Otherwise, stay in queue page
    console.log(`Staying in queue: status=${currentUserOrder.status}, position=${currentUserOrder.queuePosition}`);
  }, [currentUserOrder, router]);

  return {
    currentUserOrder,
    shouldShowQueue: currentUserOrder?.status === "pending" && currentUserOrder?.queuePosition > 1,
    shouldShowInProgress:
      currentUserOrder?.status === "preparing" ||
      currentUserOrder?.status === "brewing" ||
      (currentUserOrder?.status === "pending" && currentUserOrder?.queuePosition === 1),
    shouldShowDone: currentUserOrder?.status === "completed",
  };
}

import { useEffect } from 'react';
import { useRouter } from 'next/navigation';
import { useQueueStore } from '@/lib/queue-service';

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
      currentUserOrder.status === 'preparing' || 
      currentUserOrder.status === 'brewing';

    // If this user's order is being processed, redirect to in-progress
    if (isCurrentOrderBeingProcessed) {
      router.push('/in-progress');
      return;
    }

    // If completed, redirect to done page
    if (currentUserOrder.status === 'completed') {
      router.push('/done');
      return;
    }

  }, [currentUserOrder, router]);

  return {
    currentUserOrder,
    shouldShowQueue: currentUserOrder?.status === 'pending',
    shouldShowInProgress: currentUserOrder?.status === 'preparing' || currentUserOrder?.status === 'brewing',
    shouldShowDone: currentUserOrder?.status === 'completed'
  };
}
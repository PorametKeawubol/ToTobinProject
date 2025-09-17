// Single-order lock to prevent multiple orders being processed simultaneously
class KioskLock {
  private isLocked: boolean = false;
  private lockedOrderId?: string;
  private lockTimeout?: NodeJS.Timeout;
  private readonly LOCK_TIMEOUT_MS = 120 * 1000; // 2 minutes

  lock(orderId: string): { success: boolean; message: string } {
    if (this.isLocked) {
      return {
        success: false,
        message: `Kiosk is already locked by order: ${this.lockedOrderId}`,
      };
    }

    this.isLocked = true;
    this.lockedOrderId = orderId;

    // Auto-unlock after timeout
    this.lockTimeout = setTimeout(() => {
      this.unlock();
      console.log(`Auto-unlocked kiosk after timeout for order: ${orderId}`);
    }, this.LOCK_TIMEOUT_MS);

    return {
      success: true,
      message: `Kiosk locked for order: ${orderId}`,
    };
  }

  unlock(): { success: boolean; message: string } {
    if (!this.isLocked) {
      return {
        success: false,
        message: "Kiosk is not locked",
      };
    }

    const previousOrderId = this.lockedOrderId;
    this.isLocked = false;
    this.lockedOrderId = undefined;

    if (this.lockTimeout) {
      clearTimeout(this.lockTimeout);
      this.lockTimeout = undefined;
    }

    return {
      success: true,
      message: `Kiosk unlocked (was locked by order: ${previousOrderId})`,
    };
  }

  getStatus() {
    return {
      isLocked: this.isLocked,
      lockedOrderId: this.lockedOrderId,
      lockedSince: this.lockTimeout ? Date.now() - this.LOCK_TIMEOUT_MS : null,
    };
  }

  forceUnlock(): { success: boolean; message: string } {
    const status = this.getStatus();
    const result = this.unlock();

    return {
      success: true,
      message: `Force unlocked kiosk (was ${
        status.isLocked ? "locked" : "unlocked"
      })`,
    };
  }
}

// Export singleton instance
export const kioskLock = new KioskLock();

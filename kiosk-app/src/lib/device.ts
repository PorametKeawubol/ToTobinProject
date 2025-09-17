import type { Order } from "./schemas";

// Device control stub - will be replaced with MQTT/HTTP communication to ESP32
export class DeviceController {
  private activeOrders = new Set<string>();

  async start(orderId: string): Promise<{ success: boolean; message: string }> {
    if (this.activeOrders.has(orderId)) {
      return {
        success: false,
        message: `Order ${orderId} is already being processed`,
      };
    }

    this.activeOrders.add(orderId);

    // Mock device operation - simulate brewing time
    const brewingTimeMs = Math.random() * (30000 - 15000) + 15000; // 15-30 seconds

    console.log(
      `Starting device for order ${orderId}, estimated time: ${Math.round(
        brewingTimeMs / 1000
      )}s`
    );

    // Simulate the brewing process
    setTimeout(() => {
      this.complete(orderId);
    }, brewingTimeMs);

    return {
      success: true,
      message: `Device started for order ${orderId}`,
    };
  }

  private complete(orderId: string) {
    this.activeOrders.delete(orderId);

    // Randomly simulate success or error (90% success rate)
    const isSuccess = Math.random() > 0.1;

    if (isSuccess) {
      console.log(`Device completed order ${orderId} successfully`);
      // In a real implementation, this would update the order status via API call
      // For now, we'll handle this in the API routes
    } else {
      console.log(`Device error for order ${orderId}`);
      // Handle error case
    }
  }

  async stop(orderId: string): Promise<{ success: boolean; message: string }> {
    if (!this.activeOrders.has(orderId)) {
      return {
        success: false,
        message: `Order ${orderId} is not being processed`,
      };
    }

    this.activeOrders.delete(orderId);

    return {
      success: true,
      message: `Device stopped for order ${orderId}`,
    };
  }

  async heartbeat(): Promise<{
    status: string;
    activeOrders: string[];
    timestamp: number;
  }> {
    return {
      status: "online",
      activeOrders: Array.from(this.activeOrders),
      timestamp: Date.now(),
    };
  }

  getActiveOrders(): string[] {
    return Array.from(this.activeOrders);
  }

  isProcessing(orderId: string): boolean {
    return this.activeOrders.has(orderId);
  }
}

// Export singleton instance
export const deviceController = new DeviceController();

// TODO: Replace with MQTT client configuration
export const MQTT_CONFIG = {
  broker: process.env.MQTT_BROKER || "mqtt://localhost:1883",
  clientId: process.env.MQTT_CLIENT_ID || "kiosk_client",
  topics: {
    start: "kiosk/device/start",
    stop: "kiosk/device/stop",
    status: "kiosk/device/status",
    heartbeat: "kiosk/device/heartbeat",
  },
};

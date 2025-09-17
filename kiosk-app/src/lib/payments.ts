import generatePayload from "promptpay-qr";
import QRCode from "qrcode";
import type { PaymentStatus, CreatePaymentResult } from "./schemas";

// In-memory storage for payment status (replace with database in production)
const paymentStore = new Map<string, PaymentStatus>();

export const Payments = {
  async create(orderId: string, amount: number): Promise<CreatePaymentResult> {
    try {
      // Use the PromptPay phone number from the specification: 0984435255
      const phoneNumber = "098-443-5255";

      // Generate PromptPay payload
      const payload = generatePayload(phoneNumber, { amount });

      // Generate QR code as data URL
      const qrImageDataUrl = await QRCode.toDataURL(payload, {
        type: "image/png",
        width: 300,
        margin: 2,
        color: {
          dark: "#000000",
          light: "#FFFFFF",
        },
      });

      // Store payment as pending
      paymentStore.set(orderId, "PENDING");

      return {
        provider: "mock",
        orderId,
        amount,
        qrImageDataUrl,
        expiresInSec: 60, // 60 seconds as specified
      };
    } catch (error) {
      console.error("Error creating PromptPay QR:", error);
      throw new Error("Failed to create payment QR code");
    }
  },

  async status(orderId: string): Promise<PaymentStatus> {
    return paymentStore.get(orderId) || "PENDING";
  },

  async markAsPaid(orderId: string): Promise<void> {
    paymentStore.set(orderId, "PAID");
  },

  async markAsExpired(orderId: string): Promise<void> {
    paymentStore.set(orderId, "EXPIRED");
  },

  async markAsCancelled(orderId: string): Promise<void> {
    paymentStore.set(orderId, "CANCELLED");
  },

  // Utility function to get payment info
  getPaymentInfo(orderId: string) {
    return {
      status: paymentStore.get(orderId) || "PENDING",
      exists: paymentStore.has(orderId),
    };
  },

  // Clean up expired payments (can be called periodically)
  cleanup() {
    // In a real implementation, you'd check timestamps and clean up expired payments
    // For now, this is a placeholder
    console.log("Payment cleanup called");
  },
};

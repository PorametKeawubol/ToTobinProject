"use client";

import React, { useState, useEffect } from "react";
import { useRouter } from "next/navigation";
import { Button } from "@/components/ui/button";
import { Card } from "@/components/ui/card";
import { Progress } from "@/components/ui/progress";
import { ArrowLeft, Clock, X, CheckCircle, Users } from "lucide-react";
import { motion } from "framer-motion";
import { useCartStore, useOrderStore, useKioskStore } from "@/lib/stores";
import { useQueueStore } from "@/lib/queue-service";
import { SAMPLE_DRINKS } from "@/lib/schemas";
import type { Order } from "@/lib/schemas";

export default function CheckoutPage() {
  const router = useRouter();
  const { currentItem, clearCart } = useCartStore();
  const { setCurrentOrder } = useOrderStore();
  const { setLocked } = useKioskStore();
  const { addToQueue, currentUserOrder } = useQueueStore();

  const [isLoading, setIsLoading] = useState(true);
  const [order, setOrder] = useState<Order | null>(null);
  const [qrData, setQrData] = useState<{
    qrImageDataUrl: string;
    expiresInSec: number;
  } | null>(null);
  const [timeLeft, setTimeLeft] = useState(60);
  const [paymentStatus, setPaymentStatus] = useState<
    "waiting" | "checking" | "paid"
  >("waiting");
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    console.log("Checkout: currentItem =", currentItem); // Debug log

    // Add small delay to allow store to update
    const timer = setTimeout(() => {
      if (!currentItem) {
        console.log("No currentItem found, redirecting to home"); // Debug log
        router.push("/");
        return;
      }

      setIsLoading(false);
      createOrder();
    }, 100);

    return () => clearTimeout(timer);
  }, [currentItem, router]);

  useEffect(() => {
    if (!order || paymentStatus !== "waiting") return;

    const timer = setInterval(() => {
      setTimeLeft((prev) => {
        if (prev <= 1) {
          handleTimeout();
          return 0;
        }
        return prev - 1;
      });
    }, 1000);

    return () => clearInterval(timer);
  }, [order, paymentStatus]);

  useEffect(() => {
    if (!order || paymentStatus !== "waiting") return;

    const pollPayment = setInterval(async () => {
      try {
        const response = await fetch(`/api/payments/status/${order.id}`);
        const data = await response.json();

        if (data.status === "PAID") {
          setPaymentStatus("paid");
          await handlePaymentSuccess();
        }
      } catch (error) {
        console.error("Error checking payment status:", error);
      }
    }, 2000);

    return () => clearInterval(pollPayment);
  }, [order, paymentStatus]);

  const createOrder = async () => {
    if (!currentItem) return;

    try {
      console.log("Checkout - Creating order with:", {
        drinkId: currentItem.drink.id,
        amount: currentItem.amount,
        options: currentItem.options,
      });

      // Create order
      const orderResponse = await fetch("/api/orders/create", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          drinkId: currentItem.drink.id,
          options: currentItem.options,
          amount: currentItem.amount,
        }),
      });

      if (!orderResponse.ok) throw new Error("Failed to create order");

      const newOrder = await orderResponse.json();
      console.log("Checkout - Order created:", newOrder);

      setOrder(newOrder);
      setCurrentOrder(newOrder);

      // Create payment
      console.log("Checkout - Creating payment with amount:", newOrder.amount);

      const paymentResponse = await fetch("/api/payments/create", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          orderId: newOrder.id,
          amount: newOrder.amount,
        }),
      });

      if (!paymentResponse.ok) throw new Error("Failed to create payment");

      const paymentData = await paymentResponse.json();
      console.log("Checkout - Payment created:", paymentData);

      setQrData({
        qrImageDataUrl: paymentData.qrImageDataUrl,
        expiresInSec: paymentData.expiresInSec,
      });
    } catch (error) {
      console.error("Error creating order:", error);
      setError("ไม่สามารถสร้างคำสั่งซื้อได้ กรุณาลองใหม่");
    }
  };

  const handlePaymentSuccess = async () => {
    if (!order) return;

    try {
      console.log("Payment success - Adding to queue:", order);

      // Add to queue instead of immediate processing
      const queueOrder = await addToQueue({
        id: order.id,
        drinkName: currentItem?.drink.name || "เครื่องดื่ม",
        toppings: currentItem?.options.toppings?.map((t) => t.name) || [],
        totalAmount: order.amount,
        sessionId: Date.now().toString(),
      });

      console.log("Added to queue:", queueOrder);

      // Redirect to queue status page
      router.push("/queue");
    } catch (error) {
      console.error("Error adding to queue:", error);
      setError("เกิดข้อผิดพลาดในการเพิ่มคำสั่งลงคิว");
    }
  };

  const handleTimeout = () => {
    setError("หมดเวลาการชำระเงิน กรุณาลองใหม่");
    setTimeout(() => {
      handleCancel();
    }, 3000);
  };

  const handleCancel = () => {
    clearCart();
    router.push("/");
  };

  // Mock payment success (for testing)
  const mockPaymentSuccess = async () => {
    if (!order) return;

    try {
      await fetch("/api/payments/mock-webhook", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ orderId: order.id, status: "PAID" }),
      });
    } catch (error) {
      console.error("Error mocking payment:", error);
    }
  };

  if (isLoading) {
    return (
      <div className="min-h-screen flex items-center justify-center p-6">
        <Card className="kiosk-card text-center max-w-md">
          <div className="text-4xl mb-4">⏳</div>
          <h2 className="kiosk-text-xl mb-4">กำลังโหลด...</h2>
          <p className="text-lg text-muted-foreground">กรุณารอสักครู่</p>
        </Card>
      </div>
    );
  }

  if (!currentItem || error) {
    return (
      <div className="min-h-screen flex items-center justify-center p-6">
        <Card className="kiosk-card text-center max-w-md">
          <div className="text-4xl mb-4">❌</div>
          <h2 className="kiosk-text-xl mb-4">เกิดข้อผิดพลาด</h2>
          <p className="text-lg text-muted-foreground mb-6">
            {error || "ไม่พบข้อมูลการสั่งซื้อ"}
          </p>
          <Button onClick={handleCancel} className="kiosk-button">
            กลับไปเลือกเมนู
          </Button>
        </Card>
      </div>
    );
  }

  const drink = SAMPLE_DRINKS.find((d) => d.id === currentItem.drink.id);
  const progressPercentage = ((60 - timeLeft) / 60) * 100;

  return (
    <div className="min-h-screen p-6">
      {/* Header */}
      <div className="flex items-center justify-between mb-8">
        <Button
          variant="outline"
          size="lg"
          onClick={handleCancel}
          className="kiosk-button"
        >
          <ArrowLeft className="w-6 h-6 mr-2" />
          ยกเลิก
        </Button>

        <h1 className="text-3xl font-bold text-primary">ชำระเงิน</h1>

        <div className="w-32" />
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-8 max-w-6xl mx-auto">
        {/* Order Summary */}
        <motion.div
          initial={{ opacity: 0, x: -20 }}
          animate={{ opacity: 1, x: 0 }}
          className="space-y-6"
        >
          <Card className="kiosk-card">
            <h2 className="kiosk-text-xl mb-4">สรุปคำสั่งซื้อ</h2>

            <div className="space-y-4">
              <div className="flex items-start gap-4">
                <div className="text-4xl">🧋</div>
                <div className="flex-1">
                  <h3 className="text-xl font-semibold">{drink?.name}</h3>
                  <p className="text-muted-foreground">{drink?.description}</p>

                  <div className="mt-3 space-y-1">
                    <p>ขนาดมาตรฐาน (แก้วเดียว)</p>
                    <p>ความหวาน: {currentItem.options.sweetness}%</p>
                    {currentItem.options.toppings &&
                      currentItem.options.toppings.length > 0 && (
                        <p>
                          ท็อปปิ้ง:{" "}
                          {currentItem.options.toppings
                            .map((t) => t.name)
                            .join(", ")}
                        </p>
                      )}
                  </div>
                </div>
                <div className="text-right">
                  <div className="text-2xl font-bold">
                    ฿{currentItem.amount}
                  </div>
                </div>
              </div>
            </div>

            <div className="border-t pt-4 mt-6">
              <div className="flex justify-between items-center">
                <span className="kiosk-text-lg">รวมทั้งหมด:</span>
                <span className="text-3xl font-bold text-primary">
                  ฿{currentItem.amount}
                </span>
              </div>
            </div>
          </Card>

          {/* Order ID */}
          {order && (
            <Card className="kiosk-card">
              <div className="text-center">
                <p className="text-lg text-muted-foreground mb-2">
                  หมายเลขคำสั่งซื้อ
                </p>
                <p className="text-2xl font-mono font-bold">
                  {order.id.slice(-8).toUpperCase()}
                </p>
              </div>
            </Card>
          )}
        </motion.div>

        {/* Payment Section */}
        <motion.div
          initial={{ opacity: 0, x: 20 }}
          animate={{ opacity: 1, x: 0 }}
          className="space-y-6"
        >
          {paymentStatus === "paid" ? (
            <Card className="kiosk-card text-center">
              <motion.div
                initial={{ scale: 0 }}
                animate={{ scale: 1 }}
                className="text-6xl text-green-500 mb-4"
              >
                <CheckCircle className="w-24 h-24 mx-auto" />
              </motion.div>
              <h2 className="kiosk-text-xl text-green-600 mb-4">
                ชำระเงินสำเร็จ!
              </h2>
              <p className="text-lg">กำลังเตรียมเครื่องดื่มของคุณ...</p>
            </Card>
          ) : (
            <>
              {/* Timer */}
              <Card className="kiosk-card">
                <div className="text-center mb-4">
                  <div className="flex items-center justify-center gap-2 mb-2">
                    <Clock className="w-6 h-6 text-orange-500" />
                    <span className="kiosk-text-lg">เวลาคงเหลือ</span>
                  </div>
                  <div className="text-4xl font-bold text-orange-500 mb-3">
                    {Math.floor(timeLeft / 60)}:
                    {(timeLeft % 60).toString().padStart(2, "0")}
                  </div>
                  <Progress value={progressPercentage} className="h-3" />
                </div>
              </Card>

              {/* QR Code */}
              {qrData && (
                <Card className="kiosk-card text-center">
                  <h2 className="kiosk-text-xl mb-4">สแกน QR เพื่อชำระเงิน</h2>

                  <div className="bg-white p-4 rounded-xl inline-block mb-4">
                    <img
                      src={qrData.qrImageDataUrl}
                      alt="PromptPay QR Code"
                      className="w-64 h-64 mx-auto"
                    />
                  </div>

                  <p className="text-lg text-muted-foreground mb-2">
                    จำนวนเงิน: ฿{currentItem.amount}
                  </p>
                  <p className="text-sm text-muted-foreground">
                    สแกนด้วยแอปธนาคารของคุณ
                  </p>

                  {/* Mock Payment Button (for testing) */}
                  <Button
                    variant="outline"
                    onClick={mockPaymentSuccess}
                    className="mt-4 text-sm"
                  >
                    🧪 จำลองการชำระเงินสำเร็จ (ทดสอบ)
                  </Button>
                </Card>
              )}
            </>
          )}

          {/* Cancel Button */}
          {paymentStatus === "waiting" && (
            <Button
              variant="destructive"
              size="lg"
              onClick={handleCancel}
              className="w-full kiosk-button"
            >
              <X className="w-6 h-6 mr-2" />
              ยกเลิกคำสั่งซื้อ
            </Button>
          )}
        </motion.div>
      </div>
    </div>
  );
}

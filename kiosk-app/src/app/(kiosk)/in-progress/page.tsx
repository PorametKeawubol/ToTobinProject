"use client";

import React, { useEffect, useState } from "react";
import { useRouter } from "next/navigation";
import { Card } from "@/components/ui/card";
import { Progress } from "@/components/ui/progress";
import { motion, AnimatePresence } from "framer-motion";
import { useOrderStore, useKioskStore } from "@/lib/stores";
import { useQueueStore } from "@/lib/queue-service";
import { LEDStatusDisplay } from "@/components/LEDStatusDisplay";
import { SAMPLE_DRINKS } from "@/lib/schemas";
import type { Order } from "@/lib/schemas";

const BREWING_MESSAGES = [
  "กำลังเตรียมน้ำร้อน...",
  "กำลังชงเครื่องดื่ม...",
  "กำลังปรับรสชาติ...",
  "กำลังเติมท็อปปิ้ง...",
  "กำลังจัดเสิร์ฟ...",
];

const BREWING_STEPS = [
  { name: "preparing_cup", message: "กำลังเตรียมแก้ว", duration: 8 },
  { name: "adding_toppings", message: "ใส่ท็อปปิ้ง", duration: 10 },
  { name: "adding_ice", message: "ใส่น้ำแข็ง", duration: 7 },
  { name: "brewing_drink", message: "ใส่เครื่องดื่ม", duration: 15 },
  { name: "completed", message: "เสร็จสิ้น", duration: 5 },
];

export default function InProgressPage() {
  const router = useRouter();
  const { currentOrder, updateCurrentOrder } = useOrderStore();
  const { currentUserOrder } = useQueueStore();
  const { unlock } = useKioskStore();

  const [currentStep, setCurrentStep] = useState(0);
  const [progress, setProgress] = useState(0);
  const [estimatedTime, setEstimatedTime] = useState(45); // 45 seconds mock
  const [timeRemaining, setTimeRemaining] = useState(45);

  useEffect(() => {
    // Check if user has an order that should be in progress
    if (!currentUserOrder) {
      router.push("/");
      return;
    }

    const isProcessing =
      currentUserOrder.status === "preparing" ||
      currentUserOrder.status === "brewing";

    if (!isProcessing) {
      router.push("/queue");
      return;
    }

    // Poll for real hardware status instead of mock
    const statusInterval = setInterval(async () => {
      try {
        const response = await fetch(
          `/api/hardware/current-status?orderId=${currentUserOrder.orderId}`
        );
        const statusData = await response.json();

        if (statusData.success && statusData.order) {
          const {
            progress: hwProgress,
            currentStep: hwStep,
            message,
            estimatedTime: hwEstimatedTime,
          } = statusData.order;

          // Update current step based on hardware feedback
          const stepIndex = BREWING_STEPS.findIndex((s) => s.name === hwStep);
          if (stepIndex !== -1) {
            setCurrentStep(stepIndex);
          }

          // Use hardware-calculated progress
          setProgress(hwProgress);

          // Use hardware-calculated remaining time
          setTimeRemaining(hwEstimatedTime);

          // Check if completed
          if (statusData.order.status === "completed") {
            setProgress(100);
            setTimeRemaining(0);

            setTimeout(() => {
              unlockAndRedirect();
            }, 3000); // Wait 3 seconds before redirect
          }
        }
      } catch (error) {
        console.error("Error checking hardware status:", error);
        // Fallback to mock behavior if hardware unavailable
        console.log("Using fallback mock brewing simulation");
      }
    }, 2000); // Check every 2 seconds

    return () => clearInterval(statusInterval);
  }, [currentUserOrder, router]);

  // Function to trigger ESP32 LED completion signal
  const triggerESP32Completion = async () => {
    try {
      const response = await fetch("/api/hardware/trigger", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          hardwareId: "esp32-001",
          action: "completion_signal",
          orderId: currentUserOrder?.id,
          ledPin: 2, // LED pin for completion signal
          duration: 3000, // 3 seconds blink
        }),
      });

      if (response.ok) {
        console.log("ESP32 completion signal sent successfully");
      }
    } catch (error) {
      console.error("Error sending ESP32 completion signal:", error);
    }
  };

  const unlockAndRedirect = async () => {
    try {
      await fetch("/api/orders/lock", { method: "DELETE" });
      unlock();
      router.push("/done");
    } catch (error) {
      console.error("Error unlocking kiosk:", error);
      router.push("/done");
    }
  };

  if (!currentUserOrder) {
    return (
      <div className="min-h-screen flex items-center justify-center">
        <div className="text-center">
          <div className="text-4xl mb-4">🔄</div>
          <p className="text-xl">กำลังโหลด...</p>
        </div>
      </div>
    );
  }

  const drink = SAMPLE_DRINKS.find((d) => d.id === currentUserOrder.orderId);
  const currentStepData = BREWING_STEPS[currentStep] || BREWING_STEPS[0];
  const isCompleted = progress >= 100;

  return (
    <div className="min-h-screen flex items-center justify-center p-6 bg-gradient-to-br from-primary/5 to-secondary/10">
      <div className="w-full max-w-2xl">
        <motion.div
          initial={{ opacity: 0, scale: 0.9 }}
          animate={{ opacity: 1, scale: 1 }}
          className="text-center"
        >
          <Card className="kiosk-card">
            {/* Status Icon */}
            <div className="text-center mb-8">
              <AnimatePresence mode="wait">
                {isCompleted ? (
                  <motion.div
                    key="completed"
                    initial={{ opacity: 0, scale: 0.5 }}
                    animate={{ opacity: 1, scale: 1 }}
                    exit={{ opacity: 0, scale: 0.5 }}
                    className="text-8xl text-green-500 mb-4"
                  >
                    ✅
                  </motion.div>
                ) : (
                  <motion.div
                    key="brewing"
                    initial={{ opacity: 0, scale: 0.5 }}
                    animate={{ opacity: 1, scale: 1 }}
                    exit={{ opacity: 0, scale: 0.5 }}
                    className="text-8xl mb-4"
                  >
                    <motion.div
                      animate={{ rotate: 360 }}
                      transition={{
                        duration: 2,
                        repeat: Infinity,
                        ease: "linear",
                      }}
                    >
                      ⚙️
                    </motion.div>
                  </motion.div>
                )}
              </AnimatePresence>
            </div>

            {/* Order Info */}
            <div className="mb-8">
              <h1 className="text-3xl font-bold mb-2">
                {isCompleted ? "เสร็จแล้ว!" : "กำลังทำเครื่องดื่ม"}
              </h1>
              <div className="text-xl text-muted-foreground mb-4">
                คำสั่งที่: {currentUserOrder.id.slice(-8).toUpperCase()}
              </div>

              <div className="flex items-center justify-center gap-4 mb-6">
                <div className="text-5xl">🧋</div>
                <div className="text-left">
                  <h2 className="text-2xl font-semibold">
                    {currentUserOrder.order.drinkName}
                  </h2>
                  <p className="text-lg text-muted-foreground">
                    ขนาด {currentUserOrder.order.size} •
                    {currentUserOrder.order.toppings.length > 0 &&
                      ` ท็อปปิ้ง: ${currentUserOrder.order.toppings.join(
                        ", "
                      )}`}
                  </p>
                </div>
              </div>
            </div>

            {/* Status Message */}
            <div className="mb-8">
              <AnimatePresence mode="wait">
                <motion.div
                  key={currentStep}
                  initial={{ opacity: 0, y: 20 }}
                  animate={{ opacity: 1, y: 0 }}
                  exit={{ opacity: 0, y: -20 }}
                  transition={{ duration: 0.3 }}
                  className="text-xl"
                >
                  {isCompleted
                    ? "เครื่องดื่มของคุณพร้อมแล้ว! กรุณารับที่ช่องรับของ"
                    : currentStepData.message}
                </motion.div>
              </AnimatePresence>
            </div>

            {/* LED Status Display */}
            {!isCompleted && (
              <div className="mb-8">
                <LEDStatusDisplay orderId={currentUserOrder.orderId} />
              </div>
            )}

            {/* Progress Bar */}
            <div className="mb-8">
              <div className="flex justify-between items-center mb-2">
                <span className="text-sm text-muted-foreground">
                  ความคืบหน้า
                </span>
                <span className="text-sm font-medium">
                  {Math.round(progress)}%
                </span>
              </div>
              <Progress value={progress} className="h-4" />
              {!isCompleted && (
                <p className="text-sm text-muted-foreground mt-2">
                  เวลาที่เหลือ: {timeRemaining} วินาที
                </p>
              )}
            </div>

            {/* Instructions */}
            <div className="text-center">
              {isCompleted ? (
                <div className="space-y-2">
                  <p className="text-lg font-medium text-green-600">
                    ขอบคุณที่ใช้บริการ!
                  </p>
                  <p className="text-sm text-muted-foreground">
                    กำลังกลับสู่หน้าแรก...
                  </p>
                </div>
              ) : (
                <div className="space-y-2">
                  <p className="text-lg font-medium">
                    กรุณารอสักครู่ • ระบบจะแจ้งเตือนเมื่อเสร็จ
                  </p>
                  <p className="text-sm text-muted-foreground">
                    โปรดอย่าออกจากหน้านี้
                  </p>
                </div>
              )}
            </div>
          </Card>
        </motion.div>

        {/* Floating Animation */}
        <div className="fixed inset-0 pointer-events-none overflow-hidden">
          {[...Array(6)].map((_, i) => (
            <motion.div
              key={i}
              className="absolute text-4xl opacity-20"
              initial={{
                x: Math.random() * window.innerWidth,
                y: window.innerHeight + 100,
                rotate: 0,
              }}
              animate={{
                y: -100,
                rotate: 360,
                x: Math.random() * window.innerWidth,
              }}
              transition={{
                duration: 8 + Math.random() * 4,
                repeat: Infinity,
                delay: i * 2,
                ease: "linear",
              }}
            >
              {["☕", "🧋", "🍵", "🥤"][Math.floor(Math.random() * 4)]}
            </motion.div>
          ))}
        </div>
      </div>
    </div>
  );
}

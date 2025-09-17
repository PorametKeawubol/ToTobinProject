"use client";

import React, { useEffect, useState } from "react";
import { useRouter } from "next/navigation";
import { Card } from "@/components/ui/card";
import { Progress } from "@/components/ui/progress";
import { motion, AnimatePresence } from "framer-motion";
import { useOrderStore, useKioskStore } from "@/lib/stores";
import { SAMPLE_DRINKS } from "@/lib/schemas";
import type { Order } from "@/lib/schemas";

const BREWING_MESSAGES = [
  "กำลังเตรียมน้ำร้อน...",
  "กำลังชงเครื่องดื่ม...",
  "กำลังปรับรสชาติ...",
  "กำลังเติมท็อปปิ้ง...",
  "กำลังจัดเสิร์ฟ...",
];

export default function InProgressPage() {
  const router = useRouter();
  const { currentOrder, updateCurrentOrder } = useOrderStore();
  const { unlock } = useKioskStore();

  const [messageIndex, setMessageIndex] = useState(0);
  const [progress, setProgress] = useState(0);
  const [estimatedTime, setEstimatedTime] = useState(120); // 2 minutes

  useEffect(() => {
    if (!currentOrder) {
      router.push("/");
      return;
    }

    if (
      currentOrder.status !== "DISPENSING" &&
      currentOrder.status !== "PAID"
    ) {
      router.push("/");
      return;
    }

    // Simulate brewing progress
    const progressInterval = setInterval(() => {
      setProgress((prev) => {
        const newProgress = Math.min(prev + 1, 95); // Don't complete until order is done
        return newProgress;
      });
    }, 1000);

    // Change messages periodically
    const messageInterval = setInterval(() => {
      setMessageIndex((prev) => (prev + 1) % BREWING_MESSAGES.length);
    }, 4000);

    // Poll order status
    const statusInterval = setInterval(async () => {
      try {
        const response = await fetch(`/api/orders/status/${currentOrder.id}`);
        const updatedOrder: Order = await response.json();

        updateCurrentOrder(updatedOrder);

        if (updatedOrder.status === "DONE") {
          setProgress(100);
          setTimeout(() => {
            unlockAndRedirect();
          }, 2000);
        } else if (updatedOrder.status === "ERROR") {
          setTimeout(() => {
            unlockAndRedirect();
          }, 5000);
        }
      } catch (error) {
        console.error("Error checking order status:", error);
      }
    }, 3000);

    return () => {
      clearInterval(progressInterval);
      clearInterval(messageInterval);
      clearInterval(statusInterval);
    };
  }, [currentOrder]);

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

  if (!currentOrder) {
    return (
      <div className="min-h-screen flex items-center justify-center">
        <div className="text-center">
          <div className="text-4xl mb-4">🔄</div>
          <p className="text-xl">กำลังโหลด...</p>
        </div>
      </div>
    );
  }

  const drink = SAMPLE_DRINKS.find((d) => d.id === currentOrder.drinkId);
  const isError = currentOrder.status === "ERROR";
  const isDone = currentOrder.status === "DONE";

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
                {isError ? (
                  <motion.div
                    key="error"
                    initial={{ opacity: 0, scale: 0.5 }}
                    animate={{ opacity: 1, scale: 1 }}
                    exit={{ opacity: 0, scale: 0.5 }}
                    className="text-8xl text-red-500 mb-4"
                  >
                    ⚠️
                  </motion.div>
                ) : isDone ? (
                  <motion.div
                    key="done"
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
                {isError
                  ? "เกิดข้อผิดพลาด"
                  : isDone
                  ? "เสร็จแล้ว!"
                  : "กำลังทำเครื่องดื่ม"}
              </h1>
              <div className="text-xl text-muted-foreground mb-4">
                คำสั่งที่: {currentOrder.id.slice(-8).toUpperCase()}
              </div>

              <div className="flex items-center justify-center gap-4 mb-6">
                <div className="text-5xl">🧋</div>
                <div className="text-left">
                  <h2 className="text-2xl font-semibold">{drink?.name}</h2>
                  <p className="text-lg text-muted-foreground">
                    ขนาด {currentOrder.options.size} • หวาน{" "}
                    {currentOrder.options.sweetness}%
                  </p>
                  {currentOrder.options.toppings.length > 0 && (
                    <p className="text-sm text-muted-foreground">
                      {currentOrder.options.toppings
                        .map((t) => t.name)
                        .join(", ")}
                    </p>
                  )}
                </div>
              </div>
            </div>

            {/* Status Message */}
            <div className="mb-8">
              <AnimatePresence mode="wait">
                <motion.div
                  key={
                    isError ? "error-msg" : isDone ? "done-msg" : messageIndex
                  }
                  initial={{ opacity: 0, y: 20 }}
                  animate={{ opacity: 1, y: 0 }}
                  exit={{ opacity: 0, y: -20 }}
                  transition={{ duration: 0.3 }}
                  className="text-xl"
                >
                  {isError
                    ? "ขออภัย เกิดข้อผิดพลาดในการทำเครื่องดื่ม กรุณาติดต่อเจ้าหน้าที่"
                    : isDone
                    ? "เครื่องดื่มของคุณพร้อมแล้ว! กรุณารับที่ช่องรับของ"
                    : BREWING_MESSAGES[messageIndex]}
                </motion.div>
              </AnimatePresence>
            </div>

            {/* Progress Bar */}
            {!isError && (
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
                {!isDone && (
                  <p className="text-sm text-muted-foreground mt-2">
                    เวลาโดยประมาณ:{" "}
                    {Math.max(
                      0,
                      Math.round(((100 - progress) * estimatedTime) / 100)
                    )}{" "}
                    วินาที
                  </p>
                )}
              </div>
            )}

            {/* Instructions */}
            <div className="text-center">
              {isError ? (
                <div className="space-y-2">
                  <p className="text-lg font-medium text-red-600">
                    เราจะคืนเงินให้คุณโดยอัตโนมัติ
                  </p>
                  <p className="text-sm text-muted-foreground">
                    กำลังกลับสู่หน้าแรก...
                  </p>
                </div>
              ) : isDone ? (
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

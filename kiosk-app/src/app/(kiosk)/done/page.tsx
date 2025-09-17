"use client";

import React, { useEffect, useState } from "react";
import { useRouter } from "next/navigation";
import { Button } from "@/components/ui/button";
import { Card } from "@/components/ui/card";
import { Home, Star, ThumbsUp } from "lucide-react";
import { motion } from "framer-motion";
import { useOrderStore, useCartStore } from "@/lib/stores";
import { SAMPLE_DRINKS } from "@/lib/schemas";

export default function DonePage() {
  const router = useRouter();
  const { currentOrder, clearCurrentOrder, addToHistory } = useOrderStore();
  const { clearCart } = useCartStore();
  const [countdown, setCountdown] = useState(8);

  useEffect(() => {
    if (!currentOrder) {
      router.push("/");
      return;
    }

    // Add to history
    addToHistory(currentOrder);

    // Auto redirect countdown
    const timer = setInterval(() => {
      setCountdown((prev) => {
        if (prev <= 1) {
          handleReturnHome();
          return 0;
        }
        return prev - 1;
      });
    }, 1000);

    return () => clearInterval(timer);
  }, [currentOrder]);

  const handleReturnHome = () => {
    clearCurrentOrder();
    clearCart();
    router.push("/");
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

  return (
    <div className="min-h-screen flex items-center justify-center p-6 bg-gradient-to-br from-green-50 to-blue-50">
      <div className="w-full max-w-2xl">
        <motion.div
          initial={{ opacity: 0, scale: 0.8 }}
          animate={{ opacity: 1, scale: 1 }}
          transition={{ duration: 0.5, ease: "easeOut" }}
        >
          <Card className="kiosk-card text-center">
            {/* Success Animation */}
            <motion.div
              initial={{ scale: 0 }}
              animate={{ scale: 1 }}
              transition={{ delay: 0.2, type: "spring", stiffness: 200 }}
              className="mb-8"
            >
              <div
                className={`text-8xl mb-4 ${
                  isError ? "text-red-500" : "text-green-500"
                }`}
              >
                {isError ? "❌" : "🎉"}
              </div>
              <h1
                className={`text-4xl font-bold mb-2 ${
                  isError ? "text-red-600" : "text-green-600"
                }`}
              >
                {isError ? "เกิดข้อผิดพลาด" : "สำเร็จแล้ว!"}
              </h1>
              <p className="text-xl text-muted-foreground">
                {isError
                  ? "ขออภัยในความไม่สะดวก"
                  : "ขอบคุณที่ใช้บริการ TotoBin Kiosk"}
              </p>
            </motion.div>

            {/* Receipt */}
            <motion.div
              initial={{ opacity: 0, y: 20 }}
              animate={{ opacity: 1, y: 0 }}
              transition={{ delay: 0.4 }}
              className="bg-gray-50 rounded-xl p-6 mb-8 text-left"
            >
              <h2 className="text-xl font-semibold text-center mb-4 border-b pb-2">
                ใบเสร็จรับเงิน
              </h2>

              <div className="space-y-3">
                <div className="flex justify-between">
                  <span>หมายเลขคำสั่ง:</span>
                  <span className="font-mono">
                    {currentOrder.id.slice(-8).toUpperCase()}
                  </span>
                </div>

                <div className="flex justify-between">
                  <span>วันที่:</span>
                  <span>
                    {new Date(currentOrder.createdAt).toLocaleDateString(
                      "th-TH"
                    )}
                  </span>
                </div>

                <div className="flex justify-between">
                  <span>เวลา:</span>
                  <span>
                    {new Date(currentOrder.createdAt).toLocaleTimeString(
                      "th-TH"
                    )}
                  </span>
                </div>

                <div className="border-t pt-3">
                  <div className="flex items-start justify-between mb-2">
                    <div className="flex-1">
                      <div className="font-medium">{drink?.name}</div>
                      <div className="text-sm text-muted-foreground">
                        ขนาด {currentOrder.options.size} • หวาน{" "}
                        {currentOrder.options.sweetness}%
                      </div>
                      {currentOrder.options.toppings.length > 0 && (
                        <div className="text-sm text-muted-foreground">
                          ท็อปปิ้ง:{" "}
                          {currentOrder.options.toppings
                            .map((t) => t.name)
                            .join(", ")}
                        </div>
                      )}
                    </div>
                    <div className="text-right">
                      <div className="font-semibold">
                        ฿{currentOrder.amount}
                      </div>
                    </div>
                  </div>
                </div>

                <div className="border-t pt-3 flex justify-between font-bold text-lg">
                  <span>รวม:</span>
                  <span>฿{currentOrder.amount}</span>
                </div>

                <div className="flex justify-between text-sm text-muted-foreground">
                  <span>ชำระแล้ว:</span>
                  <span>PromptPay</span>
                </div>
              </div>

              {!isError && (
                <div className="text-center mt-4 pt-4 border-t">
                  <div className="text-sm text-muted-foreground mb-2">
                    สถานะ: เสร็จสิ้น ✓
                  </div>
                  <div className="text-xs text-muted-foreground">
                    เวลาทำ:{" "}
                    {currentOrder.finishedAt && currentOrder.paidAt
                      ? `${Math.round(
                          (currentOrder.finishedAt - currentOrder.paidAt) / 1000
                        )} วินาที`
                      : "ไม่ระบุ"}
                  </div>
                </div>
              )}
            </motion.div>

            {/* Rating */}
            {!isError && (
              <motion.div
                initial={{ opacity: 0, y: 20 }}
                animate={{ opacity: 1, y: 0 }}
                transition={{ delay: 0.6 }}
                className="mb-8"
              >
                <h3 className="text-lg font-medium mb-4">ความพึงพอใจ</h3>
                <div className="flex justify-center gap-2">
                  {[1, 2, 3, 4, 5].map((star) => (
                    <motion.button
                      key={star}
                      whileHover={{ scale: 1.1 }}
                      whileTap={{ scale: 0.95 }}
                      className="text-4xl text-yellow-400 hover:text-yellow-500 transition-colors"
                    >
                      <Star className="w-10 h-10 fill-current" />
                    </motion.button>
                  ))}
                </div>
                <p className="text-sm text-muted-foreground mt-2">
                  แตะดาวเพื่อให้คะแนน
                </p>
              </motion.div>
            )}

            {/* Call to Action */}
            <motion.div
              initial={{ opacity: 0, y: 20 }}
              animate={{ opacity: 1, y: 0 }}
              transition={{ delay: 0.8 }}
              className="space-y-4"
            >
              <Button
                size="lg"
                onClick={handleReturnHome}
                className="kiosk-button bg-primary hover:bg-primary/90 text-primary-foreground w-full"
              >
                <Home className="w-6 h-6 mr-2" />
                กลับหน้าแรก
              </Button>

              <div className="flex items-center justify-center gap-2 text-sm text-muted-foreground">
                <span>กลับสู่หน้าแรกอัตโนมัติใน</span>
                <span className="font-bold text-primary">{countdown}</span>
                <span>วินาที</span>
              </div>
            </motion.div>

            {/* Thank You Message */}
            <motion.div
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              transition={{ delay: 1 }}
              className="mt-8 pt-6 border-t"
            >
              <div className="flex items-center justify-center gap-2 text-primary">
                <ThumbsUp className="w-5 h-5" />
                <span className="font-medium">
                  {isError
                    ? "เราจะปรับปรุงบริการให้ดีขึ้น"
                    : "ขอบคุณที่เลือกใช้บริการ TotoBin!"}
                </span>
              </div>
              <p className="text-sm text-muted-foreground mt-2">
                หวังว่าจะได้เจอกันใหม่ครับ 🐢
              </p>
            </motion.div>
          </Card>
        </motion.div>

        {/* Confetti Effect */}
        {!isError && (
          <div className="fixed inset-0 pointer-events-none">
            {[...Array(20)].map((_, i) => (
              <motion.div
                key={i}
                className="absolute w-2 h-2 bg-yellow-400 rounded"
                initial={{
                  x: window.innerWidth / 2,
                  y: window.innerHeight / 2,
                  scale: 0,
                }}
                animate={{
                  x: Math.random() * window.innerWidth,
                  y: Math.random() * window.innerHeight,
                  scale: [0, 1, 0],
                }}
                transition={{
                  duration: 2,
                  delay: i * 0.1,
                  ease: "easeOut",
                }}
              />
            ))}
          </div>
        )}
      </div>
    </div>
  );
}

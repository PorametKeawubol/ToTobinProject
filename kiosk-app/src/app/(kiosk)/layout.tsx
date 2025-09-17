"use client";

import React, { useState, useEffect } from "react";
import { useKioskStore } from "@/lib/stores";
import { Coffee, Clock, Check } from "lucide-react";
import { motion } from "framer-motion";
import { useRouter } from "next/navigation";

const BREWING_STEPS = [
  { id: 1, text: "กำลังเตรียมแก้ว", icon: "🥤", duration: 3000 },
  { id: 2, text: "ใส่ท็อปปิ้ง", icon: "🧋", duration: 4000 },
  { id: 3, text: "ใส่น้ำแข็ง", icon: "🧊", duration: 3500 },
  { id: 4, text: "ใส่เครื่องดื่ม", icon: "☕", duration: 5000 },
  { id: 5, text: "เสร็จสิ้น", icon: "✅", duration: 2000 },
];

export default function KioskLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  const { isLocked, lockedOrderId, unlock } = useKioskStore();
  const [currentStep, setCurrentStep] = useState(0);
  const [isCompleted, setIsCompleted] = useState(false);
  const [timeRemaining, setTimeRemaining] = useState(45);
  const router = useRouter();

  useEffect(() => {
    if (!isLocked) {
      setCurrentStep(0);
      setIsCompleted(false);
      setTimeRemaining(45);
      return;
    }

    // Start brewing process
    let stepIndex = 0;
    const processStep = () => {
      if (stepIndex < BREWING_STEPS.length) {
        setCurrentStep(stepIndex);

        setTimeout(() => {
          stepIndex++;
          if (stepIndex >= BREWING_STEPS.length) {
            setIsCompleted(true);
            // Auto redirect to home after 3 seconds
            setTimeout(() => {
              unlock();
              router.push("/");
            }, 3000);
          } else {
            processStep();
          }
        }, BREWING_STEPS[stepIndex]?.duration || 3000);
      }
    };

    // Countdown timer
    const timer = setInterval(() => {
      setTimeRemaining((prev) => {
        if (prev <= 1) {
          clearInterval(timer);
          return 0;
        }
        return prev - 1;
      });
    }, 1000);

    processStep();

    return () => clearInterval(timer);
  }, [isLocked]);

  const formatTime = (seconds: number) => {
    const mins = Math.floor(seconds / 60);
    const secs = seconds % 60;
    return `${mins}:${secs.toString().padStart(2, "0")}`;
  };

  return (
    <div className="kiosk-container">
      {isLocked && (
        <motion.div
          initial={{ opacity: 0, scale: 0.9 }}
          animate={{ opacity: 1, scale: 1 }}
          className="fixed inset-0 z-50 bg-gradient-to-br from-gray-50 via-white to-gray-100 flex items-center justify-center"
        >
          <div className="text-center text-gray-800 max-w-2xl mx-auto p-8">
            {/* Main Header */}
            <motion.div
              initial={{ y: -50 }}
              animate={{ y: 0 }}
              className="mb-8"
            >
              <motion.div
                animate={{ rotate: 360 }}
                transition={{ duration: 3, repeat: Infinity, ease: "linear" }}
                className="inline-block mb-4"
              >
                <div className="text-8xl">🧋</div>
              </motion.div>

              <h1 className="text-5xl font-bold mb-4">กำลังทำเครื่องดื่ม</h1>
              <p className="text-2xl opacity-90 mb-2">กรุณารอสักครู่...</p>

              {/* Order ID */}
              <div className="bg-gray-200 rounded-xl p-4 shadow-md inline-block">
                <p className="text-xl font-semibold text-gray-700">
                  คำสั่งที่: #
                  {lockedOrderId?.slice(-8).toUpperCase() || "TT000001"}
                </p>
              </div>
            </motion.div>

            {/* Timer */}
            <motion.div
              initial={{ scale: 0 }}
              animate={{ scale: 1 }}
              className="mb-8"
            >
              <div className="bg-gray-200 rounded-full p-6 shadow-md inline-flex items-center gap-4">
                <motion.div
                  animate={{ scale: [1, 1.2, 1] }}
                  transition={{ duration: 1, repeat: Infinity }}
                >
                  <Clock className="h-10 w-10 text-gray-600" />
                </motion.div>
                <div className="text-left">
                  <p className="text-sm text-gray-600">เวลาที่เหลือประมาณ</p>
                  <p className="text-3xl font-bold font-mono">
                    {formatTime(timeRemaining)} นาที
                  </p>
                </div>
              </div>
            </motion.div>

            {/* Progress Steps */}
            <motion.div
              initial={{ y: 50, opacity: 0 }}
              animate={{ y: 0, opacity: 1 }}
              transition={{ delay: 0.3 }}
              className="bg-gray-200 rounded-2xl p-6 shadow-md"
            >
              <h3 className="text-xl font-semibold mb-6 text-gray-700">
                ขั้นตอนการทำ
              </h3>

              <div className="grid grid-cols-5 gap-4">
                {BREWING_STEPS.map((step, index) => {
                  const isActive = index === currentStep && !isCompleted;
                  const isStepCompleted =
                    index < currentStep ||
                    (index === currentStep && isCompleted);
                  const isPending = index > currentStep;

                  return (
                    <motion.div
                      key={step.id}
                      initial={{ opacity: 0, y: 20 }}
                      animate={{
                        opacity: 1,
                        y: 0,
                        scale: isActive ? 1.1 : 1,
                      }}
                      transition={{
                        delay: index * 0.1,
                        scale: { duration: 0.3 },
                      }}
                      className={`text-center p-4 rounded-xl transition-all duration-300 ${
                        isActive
                          ? "bg-blue-200 shadow-lg border-2 border-blue-400"
                          : isStepCompleted
                          ? "bg-green-200 border-2 border-green-400"
                          : "bg-gray-100 border-2 border-gray-300"
                      }`}
                    >
                      <div className="relative mb-3">
                        <motion.div
                          className={`text-4xl ${
                            isActive ? "animate-bounce" : ""
                          }`}
                        >
                          {step.icon}
                        </motion.div>

                        {isStepCompleted && (
                          <motion.div
                            initial={{ scale: 0, rotate: -180 }}
                            animate={{ scale: 1, rotate: 0 }}
                            className="absolute -top-1 -right-1 bg-green-500 rounded-full w-6 h-6 flex items-center justify-center"
                          >
                            <Check className="w-4 h-4 text-white" />
                          </motion.div>
                        )}
                      </div>

                      <p
                        className={`text-sm font-medium ${
                          isActive
                            ? "text-blue-800"
                            : isStepCompleted
                            ? "text-green-800"
                            : "text-gray-500"
                        }`}
                      >
                        {step.text}
                      </p>

                      {isActive && (
                        <motion.div
                          initial={{ width: 0 }}
                          animate={{ width: "100%" }}
                          transition={{ duration: step.duration / 1000 }}
                          className="mt-2 h-1 bg-blue-500 rounded-full"
                        />
                      )}
                    </motion.div>
                  );
                })}
              </div>

              {/* Status Message */}
              <motion.div
                key={currentStep}
                initial={{ opacity: 0, y: 10 }}
                animate={{ opacity: 1, y: 0 }}
                className="mt-6 p-4 bg-gray-100 rounded-xl border border-gray-300"
              >
                {isCompleted ? (
                  <p className="text-xl font-semibold text-green-700">
                    🎉 เครื่องดื่มของคุณพร้อมแล้ว!
                  </p>
                ) : (
                  <p className="text-lg text-gray-600">
                    ⏳ {BREWING_STEPS[currentStep]?.text}...
                  </p>
                )}
              </motion.div>
            </motion.div>

            {/* Animated decorations */}
            <div className="absolute top-10 left-10 opacity-30">
              <motion.div
                animate={{ y: [0, -20, 0] }}
                transition={{ duration: 2, repeat: Infinity }}
                className="text-3xl"
              >
                ☕
              </motion.div>
            </div>

            <div className="absolute top-20 right-20 opacity-30">
              <motion.div
                animate={{ y: [0, -15, 0] }}
                transition={{ duration: 2.5, repeat: Infinity }}
                className="text-3xl"
              >
                🥤
              </motion.div>
            </div>

            <div className="absolute bottom-20 left-20 opacity-30">
              <motion.div
                animate={{ y: [0, -25, 0] }}
                transition={{ duration: 1.8, repeat: Infinity }}
                className="text-3xl"
              >
                🧊
              </motion.div>
            </div>

            <div className="absolute bottom-32 right-32 opacity-30">
              <motion.div
                animate={{ y: [0, -18, 0] }}
                transition={{ duration: 2.2, repeat: Infinity }}
                className="text-3xl"
              >
                🧋
              </motion.div>
            </div>
          </div>
        </motion.div>
      )}

      <div className={`${isLocked ? "hidden" : ""}`}>{children}</div>
    </div>
  );
}

"use client";

import React, { useEffect, useState } from "react";
import { Card } from "@/components/ui/card";
import { Badge } from "@/components/ui/badge";

interface LEDState {
  preparing: boolean;
  toppings: boolean;
  ice: boolean;
  brewing: boolean;
  completed: boolean;
}

interface HardwareStatus {
  orderId: string;
  status: string;
  currentStep: string;
  progress: number;
  ledState?: LEDState;
  elapsedTime: number;
  totalTime: number;
  message: string;
}

export function LEDStatusDisplay({ orderId }: { orderId: string }) {
  const [hardwareStatus, setHardwareStatus] = useState<HardwareStatus | null>(
    null
  );
  const [isConnected, setIsConnected] = useState(false);

  useEffect(() => {
    if (!orderId) return;

    const fetchStatus = async () => {
      try {
        const response = await fetch(
          `/api/hardware/current-status?orderId=${orderId}`
        );
        const data = await response.json();

        if (data.success && data.order) {
          setHardwareStatus({
            orderId: data.order.orderId,
            status: data.order.status,
            currentStep: data.order.currentStep,
            progress: data.order.progress,
            ledState: data.order.ledState, // May be undefined if not from hardware
            elapsedTime: data.order.estimatedTime
              ? 100 - data.order.estimatedTime
              : 0,
            totalTime: 100,
            message: data.order.message,
          });
          setIsConnected(true);
        }
      } catch (error) {
        console.error("Failed to fetch hardware status:", error);
        setIsConnected(false);
      }
    };

    fetchStatus();
    const interval = setInterval(fetchStatus, 1000); // Update every second

    return () => clearInterval(interval);
  }, [orderId]);

  if (!hardwareStatus) {
    return (
      <Card className="p-4">
        <div className="text-center text-gray-500">
          กำลังโหลดสถานะฮาร์ดแวร์...
        </div>
      </Card>
    );
  }

  const ledSteps = [
    {
      key: "preparing",
      label: "เตรียมแก้ว",
      color: "bg-red-500",
      active:
        hardwareStatus.ledState?.preparing ||
        hardwareStatus.currentStep === "preparing_cup",
    },
    {
      key: "toppings",
      label: "ใส่ท็อปปิ้ง",
      color: "bg-yellow-500",
      active:
        hardwareStatus.ledState?.toppings ||
        hardwareStatus.currentStep === "adding_toppings",
    },
    {
      key: "ice",
      label: "ใส่น้ำแข็ง",
      color: "bg-blue-500",
      active:
        hardwareStatus.ledState?.ice ||
        hardwareStatus.currentStep === "adding_ice",
    },
    {
      key: "brewing",
      label: "ใส่เครื่องดื่ม",
      color: "bg-green-500",
      active:
        hardwareStatus.ledState?.brewing ||
        hardwareStatus.currentStep === "brewing_drink",
    },
    {
      key: "completed",
      label: "เสร็จสิ้น",
      color: "bg-white border-2 border-gray-300",
      active:
        hardwareStatus.ledState?.completed ||
        hardwareStatus.currentStep === "completed",
    },
  ];

  return (
    <Card className="p-6">
      <div className="space-y-4">
        {/* Connection Status */}
        <div className="flex items-center justify-between">
          <h3 className="text-lg font-semibold">สถานะเครื่องทำเครื่องดื่ม</h3>
          <Badge variant={isConnected ? "default" : "destructive"}>
            {isConnected ? "เชื่อมต่อแล้ว" : "ขาดการเชื่อมต่อ"}
          </Badge>
        </div>

        {/* Current Status */}
        <div className="text-center space-y-2">
          <div className="text-2xl font-bold text-blue-600">
            {hardwareStatus.message}
          </div>
          <div className="text-sm text-gray-600">
            ออเดอร์: {hardwareStatus.orderId}
          </div>
        </div>

        {/* LED Status Display */}
        <div className="space-y-3">
          <h4 className="font-medium text-gray-700">สถานะ LED บอร์ด</h4>
          <div className="grid grid-cols-5 gap-3">
            {ledSteps.map((step, index) => (
              <div key={step.key} className="text-center space-y-2">
                <div className="relative">
                  <div
                    className={`w-12 h-12 rounded-full mx-auto transition-all duration-300 ${
                      step.active
                        ? step.color + " shadow-lg animate-pulse"
                        : "bg-gray-200"
                    }`}
                  />
                  {step.active && (
                    <div className="absolute inset-0 w-12 h-12 rounded-full mx-auto bg-white opacity-30 animate-ping" />
                  )}
                </div>
                <div
                  className={`text-xs font-medium ${
                    step.active ? "text-blue-600" : "text-gray-400"
                  }`}
                >
                  {step.label}
                </div>
                <div
                  className={`text-xs ${
                    step.active ? "text-green-600 font-bold" : "text-gray-300"
                  }`}
                >
                  {step.active ? "ON" : "OFF"}
                </div>
              </div>
            ))}
          </div>
        </div>

        {/* Progress Information */}
        <div className="space-y-2">
          <div className="flex justify-between text-sm">
            <span>ความคืบหน้า</span>
            <span>{hardwareStatus.progress}%</span>
          </div>
          <div className="w-full bg-gray-200 rounded-full h-2">
            <div
              className="bg-blue-600 h-2 rounded-full transition-all duration-300"
              style={{ width: `${hardwareStatus.progress}%` }}
            />
          </div>
        </div>

        {/* Timing Information */}
        <div className="grid grid-cols-2 gap-4 text-sm">
          <div className="text-center p-2 bg-gray-50 rounded">
            <div className="font-medium text-gray-600">เวลาที่ใช้</div>
            <div className="text-lg font-bold text-blue-600">
              {hardwareStatus.elapsedTime}s
            </div>
          </div>
          <div className="text-center p-2 bg-gray-50 rounded">
            <div className="font-medium text-gray-600">เวลาทั้งหมด</div>
            <div className="text-lg font-bold text-gray-600">
              {hardwareStatus.totalTime}s
            </div>
          </div>
        </div>

        {/* Hardware Debug Info (Development Only) */}
        {process.env.NODE_ENV === "development" && hardwareStatus.ledState && (
          <details className="text-xs text-gray-500">
            <summary className="cursor-pointer">Hardware Debug Info</summary>
            <pre className="mt-2 p-2 bg-gray-100 rounded text-xs overflow-auto">
              {JSON.stringify(hardwareStatus.ledState, null, 2)}
            </pre>
          </details>
        )}
      </div>
    </Card>
  );
}

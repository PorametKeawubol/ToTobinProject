"use client";

import React, { useEffect, useState } from "react";
import { useRouter } from "next/navigation";
import { Button } from "@/components/ui/button";
import { Card } from "@/components/ui/card";
import { Badge } from "@/components/ui/badge";
import {
  ArrowLeft,
  Clock,
  Users,
  Coffee,
  CheckCircle,
  AlertCircle,
} from "lucide-react";
import { motion, AnimatePresence } from "framer-motion";
import { useQueueStore } from "@/lib/queue-service";
import { useRealTimeQueue, useOrderStatus } from "@/lib/hooks/useRealTimeQueue";

export default function QueuePage() {
  const router = useRouter();
  const { currentUserOrder } = useQueueStore();
  const { queueData, isConnected, error } = useRealTimeQueue();
  const orderStatus = useOrderStatus(currentUserOrder?.id || null);

  useEffect(() => {
    if (!currentUserOrder) {
      router.push("/");
      return;
    }
  }, [currentUserOrder, router]);

  const getStatusColor = (status: string) => {
    switch (status) {
      case "pending":
        return "bg-yellow-100 text-yellow-800 border-yellow-300";
      case "preparing":
        return "bg-blue-100 text-blue-800 border-blue-300";
      case "brewing":
        return "bg-orange-100 text-orange-800 border-orange-300";
      case "completed":
        return "bg-green-100 text-green-800 border-green-300";
      default:
        return "bg-gray-100 text-gray-800 border-gray-300";
    }
  };

  const getStatusText = (status: string) => {
    switch (status) {
      case "pending":
        return "รอคิว";
      case "preparing":
        return "กำลังเตรียม";
      case "brewing":
        return "กำลังทำ";
      case "completed":
        return "เสร็จแล้ว";
      default:
        return status;
    }
  };

  const getStatusIcon = (status: string) => {
    switch (status) {
      case "pending":
        return <Clock className="w-5 h-5" />;
      case "preparing":
      case "brewing":
        return <Coffee className="w-5 h-5" />;
      case "completed":
        return <CheckCircle className="w-5 h-5" />;
      default:
        return <AlertCircle className="w-5 h-5" />;
    }
  };

  if (!currentUserOrder) {
    return (
      <div className="min-h-screen flex items-center justify-center p-6">
        <Card className="kiosk-card text-center max-w-md">
          <div className="text-4xl mb-4">❓</div>
          <h2 className="kiosk-text-xl mb-4">ไม่พบคำสั่งซื้อ</h2>
          <p className="text-lg text-muted-foreground mb-6">
            กรุณาสั่งเครื่องดื่มก่อน
          </p>
          <Button onClick={() => router.push("/")} className="kiosk-button">
            กลับไปหน้าแรก
          </Button>
        </Card>
      </div>
    );
  }

  return (
    <div className="min-h-screen p-6">
      {/* Header */}
      <div className="flex items-center justify-between mb-8">
        <Button
          variant="outline"
          size="lg"
          onClick={() => router.push("/")}
          className="kiosk-button"
        >
          <ArrowLeft className="w-6 h-6 mr-2" />
          กลับหน้าแรก
        </Button>

        <h1 className="text-3xl font-bold text-primary">สถานะคิว</h1>

        <div className="flex items-center gap-2">
          <div
            className={`w-3 h-3 rounded-full ${
              isConnected ? "bg-green-500" : "bg-red-500"
            }`}
          />
          <span className="text-sm text-muted-foreground">
            {isConnected ? "เชื่อมต่อแล้ว" : "ตัดการเชื่อมต่อ"}
          </span>
        </div>
      </div>

      <div className="max-w-4xl mx-auto space-y-6">
        {/* Your Order Status */}
        <motion.div
          initial={{ opacity: 0, y: -20 }}
          animate={{ opacity: 1, y: 0 }}
        >
          <Card className="kiosk-card">
            <div className="flex items-center gap-4 mb-4">
              <div className="text-4xl">🧋</div>
              <div className="flex-1">
                <h2 className="kiosk-text-xl">คำสั่งซื้อของคุณ</h2>
                <p className="text-muted-foreground">
                  คำสั่งที่: #{currentUserOrder.id.slice(-8).toUpperCase()}
                </p>
              </div>
              <Badge
                className={`${getStatusColor(
                  orderStatus?.status || currentUserOrder.status
                )} text-lg px-4 py-2`}
              >
                <div className="flex items-center gap-2">
                  {getStatusIcon(
                    orderStatus?.status || currentUserOrder.status
                  )}
                  {getStatusText(
                    orderStatus?.status || currentUserOrder.status
                  )}
                </div>
              </Badge>
            </div>

            <div className="grid grid-cols-1 md:grid-cols-3 gap-4 mt-6">
              <div className="text-center p-4 bg-gray-50 rounded-lg">
                <div className="text-2xl font-bold text-primary">
                  #
                  {orderStatus?.queuePosition || currentUserOrder.queuePosition}
                </div>
                <p className="text-sm text-muted-foreground">ลำดับในคิว</p>
              </div>

              <div className="text-center p-4 bg-gray-50 rounded-lg">
                <div className="text-2xl font-bold text-orange-500">
                  ~
                  {orderStatus?.estimatedTime || currentUserOrder.estimatedTime}{" "}
                  นาที
                </div>
                <p className="text-sm text-muted-foreground">
                  เวลาที่คาดว่าจะรอ
                </p>
              </div>

              <div className="text-center p-4 bg-gray-50 rounded-lg">
                <div className="text-2xl font-bold text-green-500">
                  ฿{currentUserOrder.order.totalAmount}
                </div>
                <p className="text-sm text-muted-foreground">ยอดที่ชำระแล้ว</p>
              </div>
            </div>

            <div className="mt-4 p-4 bg-blue-50 rounded-lg">
              <h3 className="font-semibold mb-2">รายการที่สั่ง:</h3>
              <p>• {currentUserOrder.order.drinkName}</p>
              {currentUserOrder.order.toppings.length > 0 && (
                <p>• ท็อปปิ้ง: {currentUserOrder.order.toppings.join(", ")}</p>
              )}
            </div>
          </Card>
        </motion.div>

        {/* Queue Overview */}
        <motion.div
          initial={{ opacity: 0, y: 20 }}
          animate={{ opacity: 1, y: 0 }}
        >
          <Card className="kiosk-card">
            <div className="flex items-center gap-3 mb-6">
              <Users className="w-6 h-6 text-primary" />
              <h2 className="kiosk-text-xl">คิวทั้งหมด</h2>
              {queueData && (
                <Badge variant="secondary" className="text-lg px-3 py-1">
                  {queueData.totalInQueue} คิว
                </Badge>
              )}
            </div>

            {error && (
              <div className="mb-4 p-4 bg-red-50 border border-red-200 rounded-lg">
                <p className="text-red-700">⚠️ {error}</p>
              </div>
            )}

            <div className="space-y-3">
              <AnimatePresence>
                {queueData?.queue.map((queueItem, index) => (
                  <motion.div
                    key={queueItem.id}
                    initial={{ opacity: 0, x: -20 }}
                    animate={{ opacity: 1, x: 0 }}
                    exit={{ opacity: 0, x: 20 }}
                    transition={{ delay: index * 0.1 }}
                    className={`p-4 rounded-lg border-2 transition-all ${
                      queueItem.id === currentUserOrder.id
                        ? "border-primary bg-primary/5 shadow-md"
                        : "border-gray-200 bg-white"
                    }`}
                  >
                    <div className="flex items-center justify-between">
                      <div className="flex items-center gap-4">
                        <div className="text-2xl font-bold text-primary">
                          #{queueItem.queuePosition}
                        </div>
                        <div>
                          <p className="font-semibold">
                            {queueItem.order.drinkName}
                          </p>
                          <p className="text-sm text-muted-foreground">
                            คำสั่งที่: #{queueItem.id.slice(-8).toUpperCase()}
                          </p>
                        </div>
                      </div>

                      <div className="text-right">
                        <Badge className={getStatusColor(queueItem.status)}>
                          <div className="flex items-center gap-2">
                            {getStatusIcon(queueItem.status)}
                            {getStatusText(queueItem.status)}
                          </div>
                        </Badge>
                        <p className="text-sm text-muted-foreground mt-1">
                          ~{queueItem.estimatedTime} นาที
                        </p>
                      </div>
                    </div>

                    {queueItem.id === currentUserOrder.id && (
                      <div className="mt-3 p-3 bg-primary/10 rounded-lg">
                        <p className="text-sm font-medium text-primary">
                          👆 นี่คือคำสั่งซื้อของคุณ
                        </p>
                      </div>
                    )}
                  </motion.div>
                ))}
              </AnimatePresence>

              {queueData?.queue.length === 0 && (
                <div className="text-center py-8">
                  <div className="text-4xl mb-4">🎉</div>
                  <p className="text-lg text-muted-foreground">
                    ไม่มีคิวในขณะนี้
                  </p>
                </div>
              )}
            </div>
          </Card>
        </motion.div>

        {/* Status completed - redirect option */}
        {orderStatus?.status === "completed" && (
          <motion.div
            initial={{ opacity: 0, scale: 0.9 }}
            animate={{ opacity: 1, scale: 1 }}
          >
            <Card className="kiosk-card text-center">
              <div className="text-6xl mb-4">🎉</div>
              <h2 className="kiosk-text-xl text-green-600 mb-4">
                เครื่องดื่มของคุณพร้อมแล้ว!
              </h2>
              <p className="text-lg text-muted-foreground mb-6">
                กรุณามารับที่เครื่องทำเครื่องดื่ม
              </p>
              <Button
                onClick={() => router.push("/")}
                className="kiosk-button"
                size="lg"
              >
                สั่งเครื่องดื่มใหม่
              </Button>
            </Card>
          </motion.div>
        )}
      </div>
    </div>
  );
}

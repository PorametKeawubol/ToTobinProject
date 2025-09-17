"use client";

import React, { useState, useEffect } from "react";
import { useRouter } from "next/navigation";
import { Button } from "@/components/ui/button";
import { Card } from "@/components/ui/card";
import { ChevronLeft, ChevronRight, Coffee, Star } from "lucide-react";
import { motion, AnimatePresence } from "framer-motion";

const ADS_DATA = [
  {
    id: 1,
    title: "ชาไทยสูตรพิเศษ",
    subtitle: "หอมหวานมัน รสชาติดั้งเดิม",
    image: "🧋",
    campaign: "thai-tea-special",
    bgColor: "bg-gradient-to-br from-orange-400 to-orange-600",
  },
  {
    id: 2,
    title: "กาแฟสดใหม่",
    subtitle: "คั่วใหม่ทุกวัน หอมกรุ่น",
    image: "☕",
    campaign: "fresh-coffee",
    bgColor: "bg-gradient-to-br from-amber-700 to-amber-900",
  },
  {
    id: 3,
    title: "ชาเขียวญี่ปุ่น",
    subtitle: "เข้มข้น สดชื่น ดีต่อสุขภาพ",
    image: "🍵",
    campaign: "green-tea-promo",
    bgColor: "bg-gradient-to-br from-green-500 to-green-700",
  },
  {
    id: 4,
    title: "โปรโมชั่นพิเศษ",
    subtitle: "ซื้อ 2 แก้ว ลด 10 บาท",
    image: "🎉",
    campaign: "buy-2-save",
    bgColor: "bg-gradient-to-br from-purple-500 to-purple-700",
  },
];

export default function LandingPage() {
  const [currentAd, setCurrentAd] = useState(0);
  const router = useRouter();

  // Auto-rotate ads every 4 seconds
  useEffect(() => {
    const interval = setInterval(() => {
      setCurrentAd((prev) => (prev + 1) % ADS_DATA.length);
    }, 4000);

    return () => clearInterval(interval);
  }, []);

  const handleAdClick = (campaign?: string) => {
    const params = campaign ? `?utm_campaign=${campaign}` : "";
    router.push(`/menu${params}`);
  };

  const nextAd = () => {
    setCurrentAd((prev) => (prev + 1) % ADS_DATA.length);
  };

  const prevAd = () => {
    setCurrentAd((prev) => (prev - 1 + ADS_DATA.length) % ADS_DATA.length);
  };

  return (
    <div
      className="min-h-screen flex flex-col items-center justify-center gap-8 p-6 cursor-pointer"
      onClick={() => handleAdClick()}
    >
      {/* Header */}
      <motion.div
        initial={{ opacity: 0, y: -20 }}
        animate={{ opacity: 1, y: 0 }}
        className="text-center mb-8"
      >
        <h1 className="text-6xl font-bold text-primary mb-4">TotoBin Kiosk</h1>
        <p className="text-2xl text-muted-foreground">
          ตู้โตโต้บิน • เครื่องดื่มสดใหม่ทุกวัน
        </p>
      </motion.div>

      {/* Ad Carousel */}
      <div className="relative w-full max-w-4xl h-96">
        <AnimatePresence mode="wait">
          <motion.div
            key={currentAd}
            initial={{ opacity: 0, scale: 0.9 }}
            animate={{ opacity: 1, scale: 1 }}
            exit={{ opacity: 0, scale: 0.9 }}
            transition={{ duration: 0.3 }}
            className="absolute inset-0"
          >
            <Card
              className={`${ADS_DATA[currentAd].bgColor} h-full border-none overflow-hidden`}
            >
              <div className="h-full flex items-center justify-between p-8 text-white">
                <div className="flex-1">
                  <h2 className="text-5xl font-bold mb-4">
                    {ADS_DATA[currentAd].title}
                  </h2>
                  <p className="text-2xl opacity-90 mb-6">
                    {ADS_DATA[currentAd].subtitle}
                  </p>
                  <div className="flex items-center gap-2">
                    <Star className="w-6 h-6 fill-current" />
                    <span className="text-xl">แตะเพื่อสั่งเลย!</span>
                  </div>
                </div>
                <div className="text-9xl">{ADS_DATA[currentAd].image}</div>
              </div>
            </Card>
          </motion.div>
        </AnimatePresence>

        {/* Navigation Arrows */}
        <Button
          variant="outline"
          size="icon"
          className="absolute left-4 top-1/2 -translate-y-1/2 bg-white/20 border-white/30 text-white hover:bg-white/30 w-12 h-12"
          onClick={(e) => {
            e.stopPropagation();
            prevAd();
          }}
        >
          <ChevronLeft className="w-6 h-6" />
        </Button>

        <Button
          variant="outline"
          size="icon"
          className="absolute right-4 top-1/2 -translate-y-1/2 bg-white/20 border-white/30 text-white hover:bg-white/30 w-12 h-12"
          onClick={(e) => {
            e.stopPropagation();
            nextAd();
          }}
        >
          <ChevronRight className="w-6 h-6" />
        </Button>

        {/* Dots Indicator */}
        <div className="absolute bottom-4 left-1/2 -translate-x-1/2 flex gap-2">
          {ADS_DATA.map((_, index) => (
            <button
              key={index}
              className={`w-3 h-3 rounded-full transition-all ${
                index === currentAd ? "bg-white" : "bg-white/50"
              }`}
              onClick={(e) => {
                e.stopPropagation();
                setCurrentAd(index);
              }}
            />
          ))}
        </div>
      </div>

      {/* Call to Action */}
      <motion.div
        initial={{ opacity: 0, y: 20 }}
        animate={{ opacity: 1, y: 0 }}
        transition={{ delay: 0.3 }}
        className="text-center"
      >
        <Button
          size="lg"
          className="kiosk-button bg-primary hover:bg-primary/90 text-primary-foreground text-2xl"
          onClick={() => handleAdClick()}
        >
          <Coffee className="w-8 h-8 mr-4" />
          เริ่มสั่งเครื่องดื่ม
        </Button>

        <p className="text-lg text-muted-foreground mt-4">
          แตะหน้าจอเพื่อเริ่มต้น
        </p>
      </motion.div>

      {/* Features */}
      <div className="grid grid-cols-3 gap-6 mt-8 max-w-2xl">
        <motion.div
          initial={{ opacity: 0, y: 20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ delay: 0.4 }}
          className="text-center"
        >
          <div className="text-4xl mb-2">⚡</div>
          <p className="text-lg font-medium">ทำใหม่ทุกแก้ว</p>
        </motion.div>

        <motion.div
          initial={{ opacity: 0, y: 20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ delay: 0.5 }}
          className="text-center"
        >
          <div className="text-4xl mb-2">💳</div>
          <p className="text-lg font-medium">ชำระด้วย QR</p>
        </motion.div>

        <motion.div
          initial={{ opacity: 0, y: 20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ delay: 0.6 }}
          className="text-center"
        >
          <div className="text-4xl mb-2">🎯</div>
          <p className="text-lg font-medium">รับใน 2 นาที</p>
        </motion.div>
      </div>
    </div>
  );
}

"use client";

import React, { useState, Suspense } from "react";
import { useRouter, useSearchParams } from "next/navigation";
import { Button } from "@/components/ui/button";
import { Card } from "@/components/ui/card";
import { Badge } from "@/components/ui/badge";
import { ArrowLeft, Plus, Minus, Coffee } from "lucide-react";
import { motion } from "framer-motion";
import { useCartStore } from "@/lib/stores";
import { SAMPLE_DRINKS, SAMPLE_TOPPINGS } from "@/lib/schemas";
import type { Drink, Topping, OrderOptions } from "@/lib/schemas";

function MenuContent() {
  const router = useRouter();
  const searchParams = useSearchParams();
  const campaign = searchParams.get("utm_campaign");

  const [selectedDrink, setSelectedDrink] = useState<Drink | null>(null);
  const [options, setOptions] = useState<OrderOptions>({
    size: "M", // Fixed size for single-size kiosk
    sweetness: 50,
    toppings: [],
  });

  const { setCurrentItem } = useCartStore();

  const calculatePrice = (drink: Drink, options: OrderOptions) => {
    // Use base price without size multiplier since we have one size
    const basePrice = drink.basePrice;
    const toppingsPrice = options.toppings.reduce(
      (total, topping) => total + topping.price,
      0
    );
    return Math.round(basePrice + toppingsPrice);
  };

  const handleDrinkSelect = (drink: Drink) => {
    setSelectedDrink(drink);
    setOptions({
      size: "M", // Fixed size
      sweetness: 50,
      toppings: [],
    });
  };

  const handleToppingToggle = (topping: Topping) => {
    setOptions((prev) => ({
      ...prev,
      toppings: prev.toppings.find((t) => t.id === topping.id)
        ? prev.toppings.filter((t) => t.id !== topping.id)
        : [...prev.toppings, topping],
    }));
  };

  const handleOrderNow = () => {
    if (!selectedDrink) return;

    const amount = calculatePrice(selectedDrink, options);
    const orderData = { drink: selectedDrink, options, amount };

    setCurrentItem(selectedDrink, options, amount);

    // Backup to localStorage for reliability
    localStorage.setItem("pendingOrder", JSON.stringify(orderData));

    setSelectedDrink(null);
    router.push("/checkout");
  };

  const getSweetnessLabel = (value: number) => {
    if (value === 0) return "ไม่หวาน";
    if (value <= 25) return "หวานน้อย";
    if (value <= 50) return "หวานปกติ";
    if (value <= 75) return "หวานเข้ม";
    return "หวานมาก";
  };

  return (
    <div className="min-h-screen">
      {/* Header */}
      <div className="flex items-center justify-between p-6 bg-card border-b">
        <Button
          variant="outline"
          size="lg"
          onClick={() => router.push("/")}
          className="kiosk-button"
        >
          <ArrowLeft className="w-6 h-6 mr-2" />
          กลับหน้าแรก
        </Button>
        <div className="text-center">
          <h1 className="text-3xl font-bold text-primary">เลือกเครื่องดื่ม</h1>
          {campaign && (
            <Badge variant="secondary" className="mt-2">
              โปรโมชั่น: {campaign}
            </Badge>
          )}
        </div>
        <div className="w-48" /> {/* Spacer for centering */}
      </div>

      {/* Menu Grid */}
      <div className="p-6">
        <div className="grid grid-cols-2 md:grid-cols-3 gap-6">
          {SAMPLE_DRINKS.map((drink, index) => (
            <motion.div
              key={drink.id}
              initial={{ opacity: 0, y: 20 }}
              animate={{ opacity: 1, y: 0 }}
              transition={{ delay: index * 0.1 }}
            >
              <Card
                className="kiosk-card cursor-pointer hover:shadow-xl transition-all duration-200 transform hover:scale-105"
                onClick={() => handleDrinkSelect(drink)}
              >
                <div className="text-center">
                  <div className="text-6xl mb-4">🧋</div>
                  <h3 className="kiosk-text-xl mb-2">{drink.name}</h3>
                  <p className="text-muted-foreground mb-4 text-lg">
                    {drink.description}
                  </p>
                  <div className="text-2xl font-bold text-primary">
                    ฿
                    {selectedDrink?.id === drink.id
                      ? calculatePrice(drink, options)
                      : drink.basePrice}
                  </div>
                  <Badge variant="outline" className="mt-2">
                    ขนาด M
                  </Badge>
                </div>
              </Card>
            </motion.div>
          ))}
        </div>
      </div>

      {/* Options Modal - Custom Modal instead of Dialog */}
      {selectedDrink && (
        <div className="fixed inset-0 z-[9999] flex items-center justify-center p-4">
          {/* Backdrop */}
          <div
            className="absolute inset-0 bg-black/20"
            onClick={() => setSelectedDrink(null)}
          />

          {/* Modal Content */}
          <div className="relative bg-white border border-gray-100 rounded-2xl max-w-2xl w-full max-h-[90vh] overflow-y-auto shadow-xl z-10">
            <div className="p-8">
              <div className="text-center mb-8">
                <h2 className="text-3xl font-bold text-gray-800">
                  {selectedDrink.name}
                </h2>
              </div>

              <div className="space-y-8">
                {/* Sweetness Level */}
                <div>
                  <h3 className="text-xl font-semibold mb-6 text-gray-700">
                    ความหวาน: {getSweetnessLabel(options.sweetness)} (
                    {options.sweetness}%)
                  </h3>
                  <div className="flex items-center gap-6">
                    <Button
                      variant="outline"
                      size="icon"
                      className="h-12 w-12 rounded-full border-2 border-gray-200 hover:border-green-500 hover:bg-green-50"
                      onClick={() =>
                        setOptions((prev) => ({
                          ...prev,
                          sweetness: Math.max(0, prev.sweetness - 25),
                        }))
                      }
                      disabled={options.sweetness === 0}
                    >
                      <Minus className="w-5 h-5" />
                    </Button>

                    <div className="flex-1 bg-gray-100 rounded-full h-4 relative overflow-hidden">
                      <div
                        className="bg-gradient-to-r from-green-400 to-green-600 h-full rounded-full transition-all duration-300 ease-out"
                        style={{ width: `${options.sweetness}%` }}
                      />
                    </div>

                    <Button
                      variant="outline"
                      size="icon"
                      className="h-12 w-12 rounded-full border-2 border-gray-200 hover:border-green-500 hover:bg-green-50"
                      onClick={() =>
                        setOptions((prev) => ({
                          ...prev,
                          sweetness: Math.min(100, prev.sweetness + 25),
                        }))
                      }
                      disabled={options.sweetness === 100}
                    >
                      <Plus className="w-5 h-5" />
                    </Button>
                  </div>
                </div>

                {/* Toppings */}
                <div>
                  <h3 className="text-xl font-semibold mb-6 text-gray-700">
                    ท็อปปิ้ง:
                  </h3>
                  <div className="grid grid-cols-2 gap-4">
                    {SAMPLE_TOPPINGS.map((topping) => {
                      const isSelected = options.toppings.find(
                        (t) => t.id === topping.id
                      );
                      return (
                        <Button
                          key={topping.id}
                          variant="outline"
                          className={`p-6 h-auto flex-col rounded-xl border-2 transition-all duration-200 ${
                            isSelected
                              ? "border-green-500 bg-green-50 text-green-700"
                              : "border-gray-200 hover:border-gray-300 bg-white"
                          }`}
                          onClick={() => handleToppingToggle(topping)}
                        >
                          <span className="font-semibold text-base">
                            {topping.name}
                          </span>
                          <span className="text-sm text-gray-500 mt-1">
                            +฿{topping.price}
                          </span>
                        </Button>
                      );
                    })}
                  </div>
                </div>

                {/* Price Summary */}
                <div className="border-t border-gray-100 pt-8">
                  <div className="flex justify-between items-center mb-6">
                    <span className="text-xl font-semibold text-gray-700">
                      รวมทั้งหมด:
                    </span>
                    <span className="text-4xl font-bold text-green-600">
                      ฿{calculatePrice(selectedDrink, options)}
                    </span>
                  </div>

                  <Button
                    size="lg"
                    className="w-full h-16 text-xl font-semibold bg-green-600 hover:bg-green-700 text-white rounded-2xl shadow-lg hover:shadow-xl transition-all duration-200"
                    onClick={handleOrderNow}
                  >
                    <Coffee className="w-7 h-7 mr-3" />
                    สั่งเลย
                  </Button>
                </div>
              </div>

              {/* Close Button */}
              <button
                onClick={() => setSelectedDrink(null)}
                className="absolute top-6 right-6 text-gray-400 hover:text-gray-600 text-2xl font-light w-8 h-8 flex items-center justify-center rounded-full hover:bg-gray-100 transition-colors"
              >
                ×
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}

export default function MenuPage() {
  return (
    <Suspense fallback={<div>Loading...</div>}>
      <MenuContent />
    </Suspense>
  );
}

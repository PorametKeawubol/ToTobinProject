import { NextRequest, NextResponse } from "next/server";
import { Orders } from "@/lib/orders";
import { OrderOptions } from "@/lib/schemas";
import {
  SIZE_MULTIPLIERS,
  SAMPLE_DRINKS,
  SAMPLE_TOPPINGS,
} from "@/lib/schemas";
import { z } from "zod";

const CreateOrderRequest = z.object({
  drinkId: z.string(),
  options: OrderOptions,
  amount: z.number().optional(), // Allow amount to be passed from client
});

export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const {
      drinkId,
      options,
      amount: clientAmount,
    } = CreateOrderRequest.parse(body);

    // Find the drink
    const drink = SAMPLE_DRINKS.find((d) => d.id === drinkId);
    if (!drink) {
      return NextResponse.json({ error: "Drink not found" }, { status: 404 });
    }

    // Use client amount if provided, otherwise calculate
    let totalAmount: number;
    if (clientAmount !== undefined) {
      totalAmount = clientAmount;
    } else {
      // Calculate total amount (fallback)
      const sizeMultiplier = SIZE_MULTIPLIERS[options.size];
      const baseAmount = drink.basePrice * sizeMultiplier;
      const toppingsAmount = options.toppings.reduce((total, topping) => {
        const toppingData = SAMPLE_TOPPINGS.find((t) => t.id === topping.id);
        return total + (toppingData ? toppingData.price : 0);
      }, 0);
      totalAmount = Math.round(baseAmount + toppingsAmount);
    }

    // Create the order
    const order = Orders.create(drinkId, options, totalAmount);

    return NextResponse.json(order);
  } catch (error) {
    console.error("Error creating order:", error);
    return NextResponse.json(
      { error: "Failed to create order" },
      { status: 500 }
    );
  }
}

import { NextRequest, NextResponse } from "next/server";

// Hardware authentication middleware
const HARDWARE_API_KEY = process.env.HARDWARE_API_KEY || "dev-hardware-key";

function authenticateHardware(request: NextRequest): string | null {
  const authHeader = request.headers.get("authorization");
  const apiKey = request.headers.get("x-api-key"); // lowercase
  const apiKeyUpper = request.headers.get("X-API-Key"); // uppercase for ESP32

  if (
    authHeader?.startsWith("Bearer ") &&
    authHeader.split(" ")[1] === HARDWARE_API_KEY
  ) {
    return authHeader.split(" ")[1];
  }

  // Check both lowercase and uppercase API key headers
  if (apiKey === HARDWARE_API_KEY || apiKeyUpper === HARDWARE_API_KEY) {
    return apiKey || apiKeyUpper;
  }

  return null;
}

// Interface for ESP32 commands
interface ESP32Command {
  id: string;
  hardwareId: string;
  action: string;
  orderId?: string;
  params: {
    ledPin?: number;
    duration?: number;
  };
  timestamp: string;
  status: "pending" | "sent" | "completed";
}

// Mock database for ESP32 commands
const pendingCommands = new Map<string, ESP32Command[]>();

export async function POST(request: NextRequest) {
  const auth = authenticateHardware(request);
  if (!auth) {
    return NextResponse.json({ error: "Unauthorized" }, { status: 401 });
  }

  try {
    const body = await request.json();
    const { hardwareId, action, orderId, ledPin, duration } = body;

    if (!hardwareId || !action) {
      return NextResponse.json(
        { error: "Missing required fields" },
        { status: 400 }
      );
    }

    // Create command for ESP32
    const command: ESP32Command = {
      id: `cmd_${Date.now()}`,
      hardwareId,
      action,
      orderId,
      params: {
        ledPin: ledPin || 2,
        duration: duration || 3000,
      },
      timestamp: new Date().toISOString(),
      status: "pending" as const,
    };

    // Store command for ESP32 to poll
    if (!pendingCommands.has(hardwareId)) {
      pendingCommands.set(hardwareId, []);
    }

    const commands = pendingCommands.get(hardwareId)!;
    commands.push(command);

    console.log(`ESP32 command queued for ${hardwareId}:`, command);

    return NextResponse.json({
      success: true,
      message: "Command queued for ESP32",
      commandId: command.id,
    });
  } catch (error) {
    console.error("Error processing ESP32 trigger:", error);
    return NextResponse.json(
      { error: "Internal server error" },
      { status: 500 }
    );
  }
}

export async function GET(request: NextRequest) {
  const auth = authenticateHardware(request);
  if (!auth) {
    return NextResponse.json({ error: "Unauthorized" }, { status: 401 });
  }

  try {
    const { searchParams } = new URL(request.url);
    const hardwareId = searchParams.get("hardwareId");

    if (!hardwareId) {
      return NextResponse.json(
        { error: "Hardware ID required" },
        { status: 400 }
      );
    }

    // Get pending commands for this ESP32
    const commands = pendingCommands.get(hardwareId) || [];
    const pendingCommand = commands.find((cmd) => cmd.status === "pending");

    if (pendingCommand) {
      // Mark as sent
      pendingCommand.status = "sent";

      return NextResponse.json({
        success: true,
        command: pendingCommand,
      });
    }

    return NextResponse.json({
      success: true,
      command: null,
    });
  } catch (error) {
    console.error("Error fetching ESP32 commands:", error);
    return NextResponse.json(
      { error: "Internal server error" },
      { status: 500 }
    );
  }
}

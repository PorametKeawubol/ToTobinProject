import { NextResponse } from 'next/server';

export async function GET() {
  return NextResponse.json({
    status: 'healthy',
    timestamp: new Date().toISOString(),
    service: 'totobin-kiosk',
    environment: process.env.NODE_ENV || 'development'
  });
}
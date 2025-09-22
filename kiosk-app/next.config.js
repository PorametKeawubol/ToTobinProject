/** @type {import('next').NextConfig} */
const nextConfig = {
  // Enable standalone for production
  output: "standalone",
  
  // Experimental features
  experimental: {
    serverComponentsExternalPackages: ["@google-cloud/firestore"],
  },
  
  // Performance optimizations
  compiler: {
    removeConsole: process.env.NODE_ENV === "production",
  },
  
  // Image optimization for ODROID
  images: {
    formats: ['image/webp'],
    deviceSizes: [640, 750, 828, 1080],
    imageSizes: [16, 32, 48, 64, 96, 128],
  },
  
  // Environment variables
  env: {
    HARDWARE_API_KEY: process.env.HARDWARE_API_KEY,
    JWT_SECRET: process.env.JWT_SECRET,
    PROMPTPAY_PHONE: process.env.PROMPTPAY_PHONE,
    BUSINESS_NAME: process.env.BUSINESS_NAME,
  },
};

module.exports = nextConfig;

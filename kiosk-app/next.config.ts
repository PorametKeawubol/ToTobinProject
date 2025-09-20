import type { NextConfig } from "next";

const nextConfig: NextConfig = {
  // Enable standalone for self-hosting on Odroid
  output: "standalone",
  experimental: {
    serverComponentsExternalPackages: ["@google-cloud/firestore"],
  },
  env: {
    GOOGLE_CLOUD_PROJECT_ID: process.env.GOOGLE_CLOUD_PROJECT_ID,
    GOOGLE_CLOUD_REGION: process.env.GOOGLE_CLOUD_REGION,
    HARDWARE_API_KEY: process.env.HARDWARE_API_KEY,
    JWT_SECRET: process.env.JWT_SECRET,
    PROMPTPAY_PHONE: process.env.PROMPTPAY_PHONE,
    BUSINESS_NAME: process.env.BUSINESS_NAME,
  },
};

export default nextConfig;

import type { NextConfig } from "next";

const nextConfig: NextConfig = {
  output: "standalone",
  experimental: {
    serverComponentsExternalPackages: ["@google-cloud/firestore"],
  },
  env: {
    GOOGLE_CLOUD_PROJECT_ID: process.env.GOOGLE_CLOUD_PROJECT_ID,
    GOOGLE_CLOUD_REGION: process.env.GOOGLE_CLOUD_REGION,
    HARDWARE_API_KEY: process.env.HARDWARE_API_KEY,
    JWT_SECRET: process.env.JWT_SECRET,
  },
};

export default nextConfig;

// Use dynamic import for next-pwa to avoid type issues
let withPWA: any;

if (process.env.NODE_ENV === "production") {
  withPWA = require("next-pwa")({
    dest: "public",
    disable: false,
    register: true,
    skipWaiting: true,
    runtimeCaching: [
      {
        urlPattern: /^https?.*/,
        handler: "NetworkFirst",
        options: {
          cacheName: "offlineCache",
          expiration: {
            maxEntries: 200,
          },
        },
      },
    ],
    fallbacks: {
      document: "/offline.html",
    },
  });

  module.exports = withPWA(nextConfig);
} else {
  module.exports = nextConfig;
}

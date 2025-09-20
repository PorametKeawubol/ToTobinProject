const nextConfig = {
  // Enable standalone for production deployment
  output: "standalone",
  
  // Optimize for ARM64 architecture
  experimental: {
    serverComponentsExternalPackages: ["@google-cloud/firestore"],
  },
  
  // Production optimizations for ODROID C4
  compiler: {
    removeConsole: process.env.NODE_ENV === "production",
  },
  
  // Image optimization for low-resource environment
  images: {
    formats: ['image/webp', 'image/avif'],
    minimumCacheTTL: 60,
    deviceSizes: [640, 750, 828, 1080, 1200],
    imageSizes: [16, 32, 48, 64, 96, 128, 256],
  },
  
  // Environment variables
  env: {
    GOOGLE_CLOUD_PROJECT_ID: process.env.GOOGLE_CLOUD_PROJECT_ID,
    GOOGLE_CLOUD_REGION: process.env.GOOGLE_CLOUD_REGION,
    HARDWARE_API_KEY: process.env.HARDWARE_API_KEY,
    JWT_SECRET: process.env.JWT_SECRET,
    PROMPTPAY_PHONE: process.env.PROMPTPAY_PHONE,
    BUSINESS_NAME: process.env.BUSINESS_NAME,
  },

  // Webpack optimizations for ARM64
  webpack: (config, { isServer }) => {
    // Optimize bundle size for low-resource environment
    if (!isServer) {
      config.resolve.fallback = {
        ...config.resolve.fallback,
        fs: false,
        path: false,
        crypto: false,
      };
    }
    
    // Memory optimization
    config.optimization = {
      ...config.optimization,
      splitChunks: {
        chunks: 'all',
        cacheGroups: {
          default: {
            minChunks: 2,
            priority: -20,
            reuseExistingChunk: true,
          },
          vendor: {
            test: /[\\/]node_modules[\\/]/,
            name: 'vendors',
            priority: -10,
            chunks: 'all',
          },
        },
      },
    };
    
    return config;
  },

  // Headers for security and performance
  async headers() {
    return [
      {
        source: '/(.*)',
        headers: [
          {
            key: 'X-Frame-Options',
            value: 'SAMEORIGIN',
          },
          {
            key: 'X-Content-Type-Options',
            value: 'nosniff',
          },
          {
            key: 'X-XSS-Protection',
            value: '1; mode=block',
          },
          {
            key: 'Referrer-Policy',
            value: 'origin-when-cross-origin',
          },
        ],
      },
      {
        source: '/_next/static/(.*)',
        headers: [
          {
            key: 'Cache-Control',
            value: 'public, max-age=31536000, immutable',
          },
        ],
      },
    ];
  },
};

module.exports = nextConfig;
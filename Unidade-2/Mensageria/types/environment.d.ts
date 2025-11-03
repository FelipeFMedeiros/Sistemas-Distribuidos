/**
 * Extensão de tipos para variáveis de ambiente do Node.js
 * Fornece autocomplete e verificação de tipos para process.env
 */
declare global {
    namespace NodeJS {
        interface ProcessEnv {
            // Google Cloud Configuration
            GOOGLE_CLOUD_PROJECT_ID: string;
            TOPIC_NAME: string;
            SUBSCRIPTION_NAME_1: string;
            SUBSCRIPTION_NAME_2: string;
            GOOGLE_APPLICATION_CREDENTIALS: string;

            // Node environment
            NODE_ENV?: 'development' | 'production' | 'test';
        }
    }
}

export {};

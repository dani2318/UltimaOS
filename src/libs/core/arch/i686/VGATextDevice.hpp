#pragma once
#include <core/dev/CharacterDevice.hpp>
#include <core/arch/i686/IO.hpp>


namespace arch{
    namespace i686{
        class VGATextDevice : public CharacterDevice {
            public:
                VGATextDevice();
                virtual size_t Write(const uint8_t* data, size_t size); 
                virtual size_t Read(uint8_t* data, size_t size); 
                void Clear();
            private:
                static uint8_t* g_ScreenBuffer;
                int m_ScreenX;
                int m_ScreenY;
                void PutChar(char c);
                void Scrollback(int lines);
                void Setcursor(int x, int y);
                uint8_t GetColor(int x, int y);
                char GetChar(int x, int y);
                void PutColor(int x, int y, uint8_t color);
                void PutChar(int x, int y, char c);
        };
    }
}
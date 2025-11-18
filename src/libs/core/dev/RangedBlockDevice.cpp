#include <core/dev/RangedBlockDevice.hpp>
#include <core/cpp/Algorithm.hpp>
#include <core/Debug.hpp>

RangeBlockDevice::RangeBlockDevice()
    : m_Device(nullptr),
      m_RangeBegin(0),
      m_RangeSize(0),
      m_Position(0)
{
}

void RangeBlockDevice::Initialize(BlockDevice* device, size_t rangeBegin, size_t rangeSize)
{
    m_Device = device;
    m_RangeBegin = rangeBegin;
    m_RangeSize = rangeSize;
    m_Device->Seek(SeekPos::Set, rangeBegin * 512);
}

size_t RangeBlockDevice::Read(uint8_t* data, size_t size)
{
    if (m_Device == nullptr)
        return 0;

    // 1. Calculate the bytes to read (bounds check)
    size_t bytesToRead = min(size, Size() - Position());
    if (bytesToRead == 0)
        return 0;

    // 2. Calculate the total physical byte offset on the raw disk
    // m_RangeBegin is the partition's starting LBA (e.g., 2048)
    // m_Position is the relative byte offset (e.g., 0)
    size_t physicalOffset = (m_RangeBegin * 512) + m_Position; 
    
    // 3. CRITICAL: SEEK the underlying device to the correct physical byte offset
    if (!m_Device->Seek(SeekPos::Set, physicalOffset)) {
        // Log failure if seek fails
        return 0;
    }

    // 4. READ the data from the underlying device
    size_t bytesRead = m_Device->Read(data, bytesToRead);
    
    // 5. UPDATE internal relative position
    m_Position += bytesRead;

    return bytesRead;
}

size_t RangeBlockDevice::Write(const uint8_t* data, size_t size)
{
    if (m_Device == nullptr)
        return 0;

    size = min(size, Size() - Position());
    return m_Device->Write(data, size);
}

bool RangeBlockDevice::Seek(SeekPos pos, int rel)
{
    if (m_Device == nullptr)
        return false;
        
    switch (pos)
    {
    case SeekPos::Set:{
        Debug::Info("RangeBlockDevice", "m_RangeBegin + rel = %d", m_RangeBegin + rel);
        m_Position = rel;
        return m_Device->Seek(SeekPos::Set, m_RangeBegin + rel);
    }
    case SeekPos::Current:{
        Debug::Info("RangeBlockDevice", "rel = %d", rel);
        return m_Device->Seek(SeekPos::Current, rel);
    }

    case SeekPos::End:{
        Debug::Info("RangeBlockDevice", "m_RangeBegin + m_RangeSize = %d", m_RangeBegin + m_RangeSize);
        return m_Device->Seek(SeekPos::Set, m_RangeBegin + m_RangeSize);
    }

    default:
        return false;
    }
}

size_t RangeBlockDevice::Size()
{
    return m_RangeSize;
}

size_t RangeBlockDevice::Position()
{
    // The m_RangeBegin LBA must be converted to a byte offset before subtraction.
    // LBA * SectorSize gives the byte offset of the partition start.
    return m_Position;
}
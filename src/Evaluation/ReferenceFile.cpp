#include <SpectralEvaluation/Evaluation/ReferenceFile.h>
#include <SpectralEvaluation/Evaluation/CrossSectionData.h>
#include <SpectralEvaluation/Spectra/ReferenceSpectrumConvolution.h>

namespace Evaluation
{

CReferenceFile &CReferenceFile::operator=(const CReferenceFile &other)
{
    this->m_path                        = std::string(other.m_path);
    this->m_crossSectionFile            = std::string(other.m_crossSectionFile);
    this->m_slitFunctionFile            = std::string(other.m_slitFunctionFile);
    this->m_wavelengthCalibrationFile   = std::string(other.m_wavelengthCalibrationFile);

    this->m_specieName          = std::string(other.m_specieName);
    this->m_columnOption        = other.m_columnOption;
    this->m_columnValue         = other.m_columnValue;
    this->m_columnMaxValue      = other.m_columnMaxValue;
    this->m_shiftOption         = other.m_shiftOption;
    this->m_shiftValue          = other.m_shiftValue;
    this->m_shiftMaxValue       = other.m_shiftMaxValue;
    this->m_squeezeOption       = other.m_squeezeOption;
    this->m_squeezeValue        = other.m_squeezeValue;
    this->m_squeezeMaxValue     = other.m_squeezeMaxValue;
    this->m_isFiltered          = other.m_isFiltered;
    
    if (other.m_data != nullptr)
    {
        this->m_data.reset(new CCrossSectionData(*other.m_data));
    }

    return *this;
}

CReferenceFile::CReferenceFile(const CReferenceFile& other)
{
    this->m_path                        = std::string(other.m_path);
    this->m_crossSectionFile            = std::string(other.m_crossSectionFile);
    this->m_slitFunctionFile            = std::string(other.m_slitFunctionFile);
    this->m_wavelengthCalibrationFile   = std::string(other.m_wavelengthCalibrationFile);

    this->m_specieName      = std::string(other.m_specieName);
    this->m_columnOption    = other.m_columnOption;
    this->m_columnValue     = other.m_columnValue;
    this->m_columnMaxValue  = other.m_columnMaxValue;
    this->m_shiftOption     = other.m_shiftOption;
    this->m_shiftValue      = other.m_shiftValue;
    this->m_shiftMaxValue   = other.m_shiftMaxValue;
    this->m_squeezeOption   = other.m_squeezeOption;
    this->m_squeezeValue    = other.m_squeezeValue;
    this->m_squeezeMaxValue = other.m_squeezeMaxValue;
    this->m_isFiltered      = other.m_isFiltered;

    if (other.m_data != nullptr)
    {
        this->m_data.reset(new CCrossSectionData(*other.m_data));
    }
}

CReferenceFile::CReferenceFile(const CCrossSectionData& contents)
{
    this->m_data.reset(new CCrossSectionData(contents));
}

void CReferenceFile::SetColumn(SHIFT_TYPE option, double value, double value2)
{
    this->m_columnOption = option;
    this->m_columnValue  = value;

    if(SHIFT_LIMIT == option)
    {
        this->m_columnMaxValue = value2;
    }
}

void CReferenceFile::SetShift(SHIFT_TYPE option, double value, double value2)
{
    this->m_shiftOption = option;
    this->m_shiftValue  = value;

    if(SHIFT_LIMIT == option)
    {
        this->m_shiftMaxValue = value2;
    }
}

void CReferenceFile::SetSqueeze(SHIFT_TYPE option, double value, double value2)
{
    this->m_squeezeOption = option;
    this->m_squeezeValue  = value;

    if(SHIFT_LIMIT == option)
    {
        this->m_squeezeMaxValue = value2;
    }
}

int CReferenceFile::ReadCrossSectionDataFromFile()
{
    if (m_path.size() == 0)
    {
        return 1;
    }

    m_data.reset(new CCrossSectionData());
    if (m_data->ReadCrossSectionFile(m_path))
    {
        m_data.reset();
        return 1;
    }

    return 0;
}

int CReferenceFile::ConvolveReference()
{
    m_data.reset(new CCrossSectionData());

    if (! Evaluation::ConvolveReference(m_wavelengthCalibrationFile, m_slitFunctionFile, m_crossSectionFile, *m_data))
    {
        m_data.reset();
        return 1;
    }

    return 0;
}

}

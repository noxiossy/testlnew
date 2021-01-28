// Magic Software, Inc.
// http://www.magic-software.com
// Copyright (c) 2000-2002.  All Rights Reserved
//
// Source code from Magic Software is supplied under the terms of a license
// agreement and may not be copied or disclosed except in accordance with the
// terms of that agreement.  The various license agreements may be found at
// the Magic Software web site.  This file is subject to the license
//
// FREE SOURCE CODE
// http://www.magic-software.com/License/free.pdf

//----------------------------------------------------------------------------
inline int Interp3DBSplineUniform::Index (int iX, int iY, int iZ) const
{
    return iX + m_aiDim[0]*(iY + m_aiDim[1]*iZ);
}
//----------------------------------------------------------------------------
inline Real Interp3DBSplineUniform::operator() (Real* afX)
{
    return (*this)(afX[0],afX[1],afX[2]);
}
//----------------------------------------------------------------------------
inline Real Interp3DBSplineUniform::operator() (int* aiDx, Real* afX)
{
    return (*this)(aiDx[0],aiDx[1],aiDx[2],afX[0],afX[1],afX[2]);
}
//----------------------------------------------------------------------------

// 
// Copyright 2010 Intel Corporation
// 
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
// 
#include "applu_share.h"
#include "applu_macros.h"

void erhs(){

//   compute the right hand side based on exact solution

//c---------------------------------------------------------------------
//c  local variables
//c---------------------------------------------------------------------
      int i, j, k, m;
      int iglob, jglob;
      int iex;
      int L1, L2;
      int ist1, iend1;
      int jst1, jend1;
      double  dsspm;
      double  xi, eta, zeta;
      double  q;
      double  u21, u31, u41;
      double  tmp;
      double  u21i, u31i, u41i, u51i;
      double  u21j, u31j, u41j, u51j;
      double  u21k, u31k, u41k, u51k;
      double  u21im1, u31im1, u41im1, u51im1;
      double  u21jm1, u31jm1, u41jm1, u51jm1;
      double  u21km1, u31km1, u41km1, u51km1;

      dsspm = dssp;

      for (k=1; k<=nz; k++)
         for (j=1; j<=ny; j++)
            for (i=1; i<=nx; i++)
               for (m=1; m<=5; m++)
                  frct( m, i, j, k ) = 0.0;

      for (k=1; k<=nz; k++) {
         zeta = ( (double)(k-1) ) / ( nz - 1 );
         for (j=1; j<=ny; j++) {
            jglob = jpt + j;
            eta = ( (double)(jglob-1) ) / ( ny0 - 1 );
            for (i=1; i<=nx; i++) {
               iglob = ipt + i;
               xi = ( (double)(iglob-1) ) / ( nx0 - 1 );
               for (m=1; m<=5; m++) {
                  rsd(m,i,j,k) =  ce(m,1)
                       + ce(m,2) * xi
                       + ce(m,3) * eta
                       + ce(m,4) * zeta
                       + ce(m,5) * xi * xi
                       + ce(m,6) * eta * eta
                       + ce(m,7) * zeta * zeta
                       + ce(m,8) * xi * xi * xi
                       + ce(m,9) * eta * eta * eta
                       + ce(m,10) * zeta * zeta * zeta
                       + ce(m,11) * xi * xi * xi * xi
                       + ce(m,12) * eta * eta * eta * eta
                       + ce(m,13) * zeta * zeta * zeta * zeta;
               }
            }
         }
      }

//c---------------------------------------------------------------------
//c   xi-direction flux differences
//c---------------------------------------------------------------------
//c
//c   iex = flag : iex = 0  north/south communication
//c              : iex = 1  east/west communication
//c
//c---------------------------------------------------------------------
      iex   = 0;

//c---------------------------------------------------------------------
//c   communicate and receive/send two rows of data
//c---------------------------------------------------------------------
//
      exchange_3(rsd,iex);

      L1 = 0;
      if (north == -1) L1 = 1;
      L2 = nx + 1;
      if (south == -1) L2 = nx;

      ist1 = 1;
      iend1 = nx;
      if (north == -1) ist1 = 4;
      if (south == -1) iend1 = nx - 3;

      for (k=2; k<=nz-1; k++)
         for (j=jst; j<=jend; j++)
            for (i=L1; i<=L2; i++) {
               flux(1,i,j,k) = rsd(2,i,j,k);
               u21 = rsd(2,i,j,k) / rsd(1,i,j,k);
               q = 0.50 * (  rsd(2,i,j,k) * rsd(2,i,j,k)
                               + rsd(3,i,j,k) * rsd(3,i,j,k)
                               + rsd(4,i,j,k) * rsd(4,i,j,k) )
                            / rsd(1,i,j,k);
               flux(2,i,j,k) = rsd(2,i,j,k) * u21 + c2 * 
                               ( rsd(5,i,j,k) - q );
               flux(3,i,j,k) = rsd(3,i,j,k) * u21;
               flux(4,i,j,k) = rsd(4,i,j,k) * u21;
               flux(5,i,j,k) = ( c1 * rsd(5,i,j,k) - c2 * q ) * u21;
      }

      for (k=2; k<=nz-1; k++)
         for (j=jst; j<=jend; j++) {
            for (i=ist; i<=iend; i++)
               for (m=1; m<=5; m++)
                  frct(m,i,j,k) =  frct(m,i,j,k)
                         - tx2 * ( flux(m,i+1,j,k) - flux(m,i-1,j,k) );
            for (i=ist; i<=L2; i++) {
               tmp = 1.0 / rsd(1,i,j,k);

               u21i = tmp * rsd(2,i,j,k);
               u31i = tmp * rsd(3,i,j,k);
               u41i = tmp * rsd(4,i,j,k);
               u51i = tmp * rsd(5,i,j,k);

               tmp = 1.0 / rsd(1,i-1,j,k);

               u21im1 = tmp * rsd(2,i-1,j,k);
               u31im1 = tmp * rsd(3,i-1,j,k);
               u41im1 = tmp * rsd(4,i-1,j,k);
               u51im1 = tmp * rsd(5,i-1,j,k);

               flux(2,i,j,k) = (4.0/3.0) * tx3 * 
                              ( u21i - u21im1 );
               flux(3,i,j,k) = tx3 * ( u31i - u31im1 );
               flux(4,i,j,k) = tx3 * ( u41i - u41im1 );
               flux(5,i,j,k) = 0.50 * ( 1.0 - c1*c5 )
                    * tx3 * ( ( u21i*u21i + u31i*u31i + u41i*u41i )
                            - ( u21im1*u21im1 + u31im1*u31im1 + u41im1*u41im1 ) )
                    + (1.0/6.0)
                    * tx3 * ( u21i*u21i - u21im1*u21im1 )
                    + c1 * c5 * tx3 * ( u51i - u51im1 );
            }

            for (i=ist; i<=iend; i++) {
               frct(1,i,j,k) = frct(1,i,j,k)
                    + dx1 * tx1 * (            rsd(1,i-1,j,k)
                                   - 2.0 * rsd(1,i,j,k)
                                   +           rsd(1,i+1,j,k) );
               frct(2,i,j,k) = frct(2,i,j,k)
                 + tx3 * c3 * c4 * ( flux(2,i+1,j,k) - flux(2,i,j,k) )
                    + dx2 * tx1 * (            rsd(2,i-1,j,k)
                                   - 2.0 * rsd(2,i,j,k)
                                   +           rsd(2,i+1,j,k) );
               frct(3,i,j,k) = frct(3,i,j,k)
                 + tx3 * c3 * c4 * ( flux(3,i+1,j,k) - flux(3,i,j,k) )
                    + dx3 * tx1 * (            rsd(3,i-1,j,k)
                                   - 2.0 * rsd(3,i,j,k)
                                   +           rsd(3,i+1,j,k) );
               frct(4,i,j,k) = frct(4,i,j,k)
                  + tx3 * c3 * c4 * ( flux(4,i+1,j,k) - flux(4,i,j,k) )
                    + dx4 * tx1 * (            rsd(4,i-1,j,k)
                                   - 2.0 * rsd(4,i,j,k)
                                   +           rsd(4,i+1,j,k) );
               frct(5,i,j,k) = frct(5,i,j,k)
                 + tx3 * c3 * c4 * ( flux(5,i+1,j,k) - flux(5,i,j,k) )
                    + dx5 * tx1 * (            rsd(5,i-1,j,k)
                                   - 2.0 * rsd(5,i,j,k)
                                   +           rsd(5,i+1,j,k) );
            }

//c---------------------------------------------------------------------
//c   Fourth-order dissipation
//c---------------------------------------------------------------------
            if (north == -1) {
             for (m=1; m<=5; m++) {
               frct(m,2,j,k) = frct(m,2,j,k)
                 - dsspm * ( + 5.0 * rsd(m,2,j,k)
                             - 4.0 * rsd(m,3,j,k)
                             +           rsd(m,4,j,k) );
               frct(m,3,j,k) = frct(m,3,j,k)
                 - dsspm * ( - 4.0 * rsd(m,2,j,k)
                             + 6.0 * rsd(m,3,j,k)
                             - 4.0 * rsd(m,4,j,k)
                             +           rsd(m,5,j,k) );
             }
            }

            for (i=ist1; i<=iend1; i++)
               for (m=1; m<=5; m++)
                  frct(m,i,j,k) = frct(m,i,j,k) 
                    - dsspm * (            rsd(m,i-2,j,k)
                               - 4.0 * rsd(m,i-1,j,k)
                               + 6.0 * rsd(m,i,j,k)
                               - 4.0 * rsd(m,i+1,j,k)
                               +           rsd(m,i+2,j,k) );

            if (south == -1) {
             for (m=1; m<=5; m++) {
               frct(m,nx-2,j,k) = frct(m,nx-2,j,k)
                 - dsspm * (             rsd(m,nx-4,j,k)
                             - 4.0 * rsd(m,nx-3,j,k)
                             + 6.0 * rsd(m,nx-2,j,k)
                             - 4.0 * rsd(m,nx-1,j,k)  );
               frct(m,nx-1,j,k) = frct(m,nx-1,j,k)
                 - dsspm * (             rsd(m,nx-3,j,k)
                             - 4.0 * rsd(m,nx-2,j,k)
                             + 5.0 * rsd(m,nx-1,j,k) );
             }
            }

         }

//c---------------------------------------------------------------------
//c   eta-direction flux differences
//c---------------------------------------------------------------------
//c
//c   iex = flag : iex = 0  north/south communication
//c              : iex = 1  east/west communication
//c
//c---------------------------------------------------------------------
      iex   = 1;

//c---------------------------------------------------------------------
//c   communicate and receive/send two rows of data
//c---------------------------------------------------------------------

      exchange_3(rsd,iex);

      L1 = 0;
      if (west == -1) L1 = 1;
      L2 = ny + 1;
      if (east == -1) L2 = ny;

      jst1 = 1;
      jend1 = ny;
      if (west == -1) jst1 = 4;
      if (east == -1) jend1 = ny - 3;

      for (k=2; k<=nz-1; k++)
         for (j=L1; j<=L2; j++)
            for (i=ist; i<=iend; i++) {
               flux(1,i,j,k) = rsd(3,i,j,k);
               u31 = rsd(3,i,j,k) / rsd(1,i,j,k);
               q = 0.50 * (  rsd(2,i,j,k) * rsd(2,i,j,k)
                               + rsd(3,i,j,k) * rsd(3,i,j,k)
                               + rsd(4,i,j,k) * rsd(4,i,j,k) )
                            / rsd(1,i,j,k);
               flux(2,i,j,k) = rsd(2,i,j,k) * u31 ;
               flux(3,i,j,k) = rsd(3,i,j,k) * u31 + c2 * 
                             ( rsd(5,i,j,k) - q );
               flux(4,i,j,k) = rsd(4,i,j,k) * u31;
               flux(5,i,j,k) = ( c1 * rsd(5,i,j,k) - c2 * q ) * u31;
      }

      for (k=2; k<=nz-1; k++) {
         for (i=ist; i<=iend; i++) 
            for (j=jst; j<=jend; j++) 
               for (m=1; m<=5; m++)
                  frct(m,i,j,k) =  frct(m,i,j,k)
                       - ty2 * ( flux(m,i,j+1,k) - flux(m,i,j-1,k) );

         for (j=jst; j<=L2; j++)
            for (i=ist; i<=iend; i++) {
               tmp = 1.0 / rsd(1,i,j,k);

               u21j = tmp * rsd(2,i,j,k);
               u31j = tmp * rsd(3,i,j,k);
               u41j = tmp * rsd(4,i,j,k);
               u51j = tmp * rsd(5,i,j,k);

               tmp = 1.0 / rsd(1,i,j-1,k);

               u21jm1 = tmp * rsd(2,i,j-1,k);
               u31jm1 = tmp * rsd(3,i,j-1,k);
               u41jm1 = tmp * rsd(4,i,j-1,k);
               u51jm1 = tmp * rsd(5,i,j-1,k);

               flux(2,i,j,k) = ty3 * ( u21j - u21jm1 );
               flux(3,i,j,k) = (4.0/3.0) * ty3 * 
                             ( u31j - u31jm1 );
               flux(4,i,j,k) = ty3 * ( u41j - u41jm1 );
               flux(5,i,j,k) = 0.50 * ( 1.0 - c1*c5 )
                    * ty3 * ( (   u21j*u21j +   u31j*u31j +   u41j*u41j )
                            - ( u21jm1*u21jm1 + u31jm1*u31jm1 + u41jm1*u41jm1 ) )
                    + (1.0/6.0)
                    * ty3 * ( u31j*u31j - u31jm1*u31jm1 )
                    + c1 * c5 * ty3 * ( u51j - u51jm1 );
         }

         for (j=jst; j<=jend; j++) 
            for (i=ist; i<=iend; i++) {
               frct(1,i,j,k) = frct(1,i,j,k)
                    + dy1 * ty1 * (            rsd(1,i,j-1,k)
                                   - 2.0 * rsd(1,i,j,k)
                                   +           rsd(1,i,j+1,k) );
               frct(2,i,j,k) = frct(2,i,j,k)
                + ty3 * c3 * c4 * ( flux(2,i,j+1,k) - flux(2,i,j,k) )
                    + dy2 * ty1 * (            rsd(2,i,j-1,k)
                                   - 2.0 * rsd(2,i,j,k)
                                   +           rsd(2,i,j+1,k) );
               frct(3,i,j,k) = frct(3,i,j,k)
                + ty3 * c3 * c4 * ( flux(3,i,j+1,k) - flux(3,i,j,k) )
                    + dy3 * ty1 * (            rsd(3,i,j-1,k)
                                   - 2.0 * rsd(3,i,j,k)
                                   +           rsd(3,i,j+1,k) );
               frct(4,i,j,k) = frct(4,i,j,k)
                + ty3 * c3 * c4 * ( flux(4,i,j+1,k) - flux(4,i,j,k) )
                    + dy4 * ty1 * (            rsd(4,i,j-1,k)
                                   - 2.0 * rsd(4,i,j,k)
                                   +           rsd(4,i,j+1,k) );
               frct(5,i,j,k) = frct(5,i,j,k)
                + ty3 * c3 * c4 * ( flux(5,i,j+1,k) - flux(5,i,j,k) )
                    + dy5 * ty1 * (            rsd(5,i,j-1,k)
                                   - 2.0 * rsd(5,i,j,k)
                                   +           rsd(5,i,j+1,k) );
         }

//c---------------------------------------------------------------------
//c   fourth-order dissipation
//c---------------------------------------------------------------------
         if (west == -1) {
            for (i=ist; i<=iend; i++) 
             for (m=1; m<=5; m++) {
               frct(m,i,2,k) = frct(m,i,2,k)
                 - dsspm * ( + 5.0 * rsd(m,i,2,k)
                             - 4.0 * rsd(m,i,3,k)
                             +           rsd(m,i,4,k) );
               frct(m,i,3,k) = frct(m,i,3,k)
                 - dsspm * ( - 4.0 * rsd(m,i,2,k)
                             + 6.0 * rsd(m,i,3,k)
                             - 4.0 * rsd(m,i,4,k)
                             +           rsd(m,i,5,k) );
            }
         }

         for (j=jst1; j<=jend1; j++)
            for (i=ist; i<=iend; i++) 
               for (m=1; m<=5; m++) 
                  frct(m,i,j,k) = frct(m,i,j,k)
                    - dsspm * (            rsd(m,i,j-2,k)
                              - 4.0 * rsd(m,i,j-1,k)
                              + 6.0 * rsd(m,i,j,k)
                              - 4.0 * rsd(m,i,j+1,k)
                              +           rsd(m,i,j+2,k) );

         if (east == -1) {
            for (i=ist; i<=iend; i++) 
             for (m=1; m<=5; m++) {
               frct(m,i,ny-2,k) = frct(m,i,ny-2,k)
                 - dsspm * (             rsd(m,i,ny-4,k)
                             - 4.0 * rsd(m,i,ny-3,k)
                             + 6.0 * rsd(m,i,ny-2,k)
                             - 4.0 * rsd(m,i,ny-1,k)  );
               frct(m,i,ny-1,k) = frct(m,i,ny-1,k)
                 - dsspm * (             rsd(m,i,ny-3,k)
                             - 4.0 * rsd(m,i,ny-2,k)
                             + 5.0 * rsd(m,i,ny-1,k)  );
             }
         }

      }

//c---------------------------------------------------------------------
//c   zeta-direction flux differences
//c---------------------------------------------------------------------
      for (k=1; k<=nz; k++) 
         for (j=jst; j<=jend; j++) 
            for (i=ist; i<=iend; i++) {
               flux(1,i,j,k) = rsd(4,i,j,k);
               u41 = rsd(4,i,j,k) / rsd(1,i,j,k);
               q = 0.50 * (  rsd(2,i,j,k) * rsd(2,i,j,k)
                               + rsd(3,i,j,k) * rsd(3,i,j,k)
                               + rsd(4,i,j,k) * rsd(4,i,j,k) )
                            / rsd(1,i,j,k);
               flux(2,i,j,k) = rsd(2,i,j,k) * u41 ;
               flux(3,i,j,k) = rsd(3,i,j,k) * u41 ;
               flux(4,i,j,k) = rsd(4,i,j,k) * u41 + c2 * 
                               ( rsd(5,i,j,k) - q );
               flux(5,i,j,k) = ( c1 * rsd(5,i,j,k) - c2 * q ) * u41;
      }

      for (k=2; k<=nz-1; k++)
         for (j=jst; j<=jend; j++) 
            for (i=ist; i<=iend; i++) 
               for (m=1; m<=5; m++)
                  frct(m,i,j,k) =  frct(m,i,j,k)
                        - tz2 * ( flux(m,i,j,k+1) - flux(m,i,j,k-1) );

      for (k=2; k<=nz; k++) 
         for (j=jst; j<=jend; j++) 
            for (i=ist; i<=iend; i++) {
               tmp = 1.0 / rsd(1,i,j,k);

               u21k = tmp * rsd(2,i,j,k);
               u31k = tmp * rsd(3,i,j,k);
               u41k = tmp * rsd(4,i,j,k);
               u51k = tmp * rsd(5,i,j,k);

               tmp = 1.0 / rsd(1,i,j,k-1);

               u21km1 = tmp * rsd(2,i,j,k-1);
               u31km1 = tmp * rsd(3,i,j,k-1);
               u41km1 = tmp * rsd(4,i,j,k-1);
               u51km1 = tmp * rsd(5,i,j,k-1);

               flux(2,i,j,k) = tz3 * ( u21k - u21km1 );
               flux(3,i,j,k) = tz3 * ( u31k - u31km1 );
               flux(4,i,j,k) = (4.0/3.0) * tz3 * ( u41k 
                             - u41km1 );
               flux(5,i,j,k) = 0.50 * ( 1.0 - c1*c5 )
                    * tz3 * ( (   u21k*u21k +   u31k*u31k +   u41k*u41k )
                            - ( u21km1*u21km1 + u31km1*u31km1 + u41km1*u41km1 ) )
                    + (1.0/6.0)
                    * tz3 * ( u41k*u41k - u41km1*u41km1 )
                    + c1 * c5 * tz3 * ( u51k - u51km1 );
      }

      for (k=2; k<=nz-1; k++)
         for (j=jst; j<=jend; j++) 
            for (i=ist; i<=iend; i++) {
               frct(1,i,j,k) = frct(1,i,j,k)
                    + dz1 * tz1 * (            rsd(1,i,j,k+1)
                                   - 2.0 * rsd(1,i,j,k)
                                   +           rsd(1,i,j,k-1) );
               frct(2,i,j,k) = frct(2,i,j,k)
                + tz3 * c3 * c4 * ( flux(2,i,j,k+1) - flux(2,i,j,k) )
                    + dz2 * tz1 * (            rsd(2,i,j,k+1)
                                   - 2.0 * rsd(2,i,j,k)
                                   +           rsd(2,i,j,k-1) );
               frct(3,i,j,k) = frct(3,i,j,k)
                + tz3 * c3 * c4 * ( flux(3,i,j,k+1) - flux(3,i,j,k) )
                    + dz3 * tz1 * (            rsd(3,i,j,k+1)
                                   - 2.0 * rsd(3,i,j,k)
                                   +           rsd(3,i,j,k-1) );
               frct(4,i,j,k) = frct(4,i,j,k)
                + tz3 * c3 * c4 * ( flux(4,i,j,k+1) - flux(4,i,j,k) )
                    + dz4 * tz1 * (            rsd(4,i,j,k+1)
                                   - 2.0 * rsd(4,i,j,k)
                                   +           rsd(4,i,j,k-1) );
               frct(5,i,j,k) = frct(5,i,j,k)
                + tz3 * c3 * c4 * ( flux(5,i,j,k+1) - flux(5,i,j,k) )
                    + dz5 * tz1 * (            rsd(5,i,j,k+1)
                                   - 2.0 * rsd(5,i,j,k)
                                   +           rsd(5,i,j,k-1) );
      }

//c---------------------------------------------------------------------
//c   fourth-order dissipation
//c---------------------------------------------------------------------
      for (j=jst; j<=jend; j++) 
         for (i=ist; i<=iend; i++) 
            for (m=1; m<=5; m++) {
               frct(m,i,j,2) = frct(m,i,j,2)
                 - dsspm * ( + 5.0 * rsd(m,i,j,2)
                             - 4.0 * rsd(m,i,j,3)
                             +           rsd(m,i,j,4) );
               frct(m,i,j,3) = frct(m,i,j,3)
                 - dsspm * (- 4.0 * rsd(m,i,j,2)
                            + 6.0 * rsd(m,i,j,3)
                            - 4.0 * rsd(m,i,j,4)
                            +           rsd(m,i,j,5) );
      }

      for (k=4; k<=nz-3; k++)
         for (j=jst; j<=jend; j++) 
            for (i=ist; i<=iend; i++) 
               for (m=1; m<=5; m++) 
                  frct(m,i,j,k) = frct(m,i,j,k)
                    - dsspm * (           rsd(m,i,j,k-2)
                              - 4.0 * rsd(m,i,j,k-1)
                              + 6.0 * rsd(m,i,j,k)
                              - 4.0 * rsd(m,i,j,k+1)
                              +           rsd(m,i,j,k+2) );

      for (j=jst; j<=jend; j++) 
         for (i=ist; i<=iend; i++) 
            for (m=1; m<=5; m++) {
               frct(m,i,j,nz-2) = frct(m,i,j,nz-2)
                 - dsspm * (            rsd(m,i,j,nz-4)
                            - 4.0 * rsd(m,i,j,nz-3)
                            + 6.0 * rsd(m,i,j,nz-2)
                            - 4.0 * rsd(m,i,j,nz-1)  );
               frct(m,i,j,nz-1) = frct(m,i,j,nz-1)
                 - dsspm * (             rsd(m,i,j,nz-3)
                             - 4.0 * rsd(m,i,j,nz-2)
                             + 5.0 * rsd(m,i,j,nz-1)  );
      }

      return;
}


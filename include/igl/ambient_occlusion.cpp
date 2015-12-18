// This file is part of libigl, a simple c++ geometry processing library.
// 
// Copyright (C) 2015 Alec Jacobson <alecjacobson@gmail.com>
// 
// This Source Code Form is subject to the terms of the Mozilla Public License 
// v. 2.0. If a copy of the MPL was not distributed with this file, You can 
// obtain one at http://mozilla.org/MPL/2.0/.
#include "ambient_occlusion.h"
#include "random_dir.h"
#include "ray_mesh_intersect.h"
#include "EPS.h"
#include "Hit.h"

template <
  typename DerivedP,
  typename DerivedN,
  typename DerivedS >
IGL_INLINE void igl::ambient_occlusion(
  const std::function<
    bool(
      const Eigen::Vector3f&,
      const Eigen::Vector3f&)
      > & shoot_ray,
  const Eigen::PlainObjectBase<DerivedP> & P,
  const Eigen::PlainObjectBase<DerivedN> & N,
  const int num_samples,
  Eigen::PlainObjectBase<DerivedS> & S)
{
  using namespace Eigen;
  using namespace igl;
  const int n = P.rows();
  // Resize output
  S.resize(n,1);
  VectorXi hits = VectorXi::Zero(n,1);
  // Embree seems to be parallel when constructing but not when tracing rays
#pragma omp parallel for
  const MatrixXf D = random_dir_stratified(num_samples).cast<float>();
  // loop over mesh vertices
  for(int p = 0;p<n;p++)
  {
    const Vector3f origin = P.row(p).template cast<float>();
    const Vector3f normal = N.row(p).template cast<float>();
    int num_hits = 0;
    for(int s = 0;s<num_samples;s++)
    {
//      //Vector3d d = random_dir();
      Vector3f d = D.row(s);
      if(d.dot(normal) < 0)
      {
        // reverse ray
        d *= -1;
      }
      if(shoot_ray(origin,d))
      {
        num_hits++;
      }
    }
    S(p) = (double)num_hits/(double)num_samples;
  }
}

template <
  typename DerivedV,
  int DIM,
  typename DerivedF,
  typename DerivedP,
  typename DerivedN,
  typename DerivedS >
IGL_INLINE void igl::ambient_occlusion(
  const igl::AABB<DerivedV,DIM> & aabb,
  const Eigen::PlainObjectBase<DerivedV> & V,
  const Eigen::PlainObjectBase<DerivedF> & F,
  const Eigen::PlainObjectBase<DerivedP> & P,
  const Eigen::PlainObjectBase<DerivedN> & N,
  const int num_samples,
  Eigen::PlainObjectBase<DerivedS> & S)
{
  const auto & shoot_ray = [&aabb,&V,&F](
    const Eigen::Vector3f& _s,
    const Eigen::Vector3f& dir)->bool
  {
    Eigen::Vector3f s = _s+1e-4*dir;
    igl::Hit hit;
    return aabb.intersect_ray(
      V,
      F,
      s  .cast<typename DerivedV::Scalar>().eval(),
      dir.cast<typename DerivedV::Scalar>().eval(),
      hit);
  };
  return ambient_occlusion(shoot_ray,P,N,num_samples,S);

}

template <
  typename DerivedV,
  typename DerivedF,
  typename DerivedP,
  typename DerivedN,
  typename DerivedS >
IGL_INLINE void igl::ambient_occlusion(
  const Eigen::PlainObjectBase<DerivedV> & V,
  const Eigen::PlainObjectBase<DerivedF> & F,
  const Eigen::PlainObjectBase<DerivedP> & P,
  const Eigen::PlainObjectBase<DerivedN> & N,
  const int num_samples,
  Eigen::PlainObjectBase<DerivedS> & S)
{
  if(F.rows() < 100)
  {
    // Super naive
    const auto & shoot_ray = [&V,&F](
      const Eigen::Vector3f& _s,
      const Eigen::Vector3f& dir)->bool
    {
      Eigen::Vector3f s = _s+1e-4*dir;
      igl::Hit hit;
      return ray_mesh_intersect(s,dir,V,F,hit);
    };
    return ambient_occlusion(shoot_ray,P,N,num_samples,S);
  }
  AABB<DerivedV,3> aabb;
  aabb.init(V,F);
  return ambient_occlusion(aabb,V,F,P,N,num_samples,S);
}

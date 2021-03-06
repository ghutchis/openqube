/******************************************************************************

  This source file is part of the OpenQube project.

  Copyright 2008-2010 Marcus D. Hanwell

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

******************************************************************************/

#include "cube.h"

#include "molecule.h"

#include <QtCore/QReadWriteLock>
#include <QtCore/QDebug>

namespace OpenQube {

using Eigen::Vector3i;
using Eigen::Vector3f;
using Eigen::Vector3d;

Cube::Cube() : m_data(0),
  m_min(0.0, 0.0, 0.0), m_max(0.0, 0.0, 0.0), m_spacing(0.0, 0.0, 0.0),
  m_points(0, 0, 0), m_minValue(0.0), m_maxValue(0.0),
  m_lock(new QReadWriteLock)
{
}

Cube::~Cube()
{
  delete m_lock;
  m_lock = 0;
}

void Cube::min(double pos[3]) const
{
  Eigen::Vector3d epos = this->min();
  pos[0] = epos.x();
  pos[1] = epos.y();
  pos[2] = epos.z();
}

void Cube::max(double pos[3]) const
{
  Eigen::Vector3d epos = this->max();
  pos[0] = epos.x();
  pos[1] = epos.y();
  pos[2] = epos.z();
}

void Cube::spacing(double pos[3]) const
{
  Eigen::Vector3d epos = this->spacing();
  pos[0] = epos.x();
  pos[1] = epos.y();
  pos[2] = epos.z();
}

void Cube::dimensions(int pos[3]) const
{
  Eigen::Vector3i epos = this->dimensions();
  pos[0] = epos.x();
  pos[1] = epos.y();
  pos[2] = epos.z();
}

bool Cube::setLimits(const Vector3d &min, const Vector3d &max,
                     const Vector3i &points)
{
  // We can calculate all necessary properties and initialise our data
  Vector3d delta = max - min;
  m_spacing = Vector3d(delta.x() / (points.x()-1),
                       delta.y() / (points.y()-1),
                       delta.z() / (points.z()-1));
  m_min = min;
  m_max = max;
  m_points = points;
  m_data.resize(m_points.x() * m_points.y() * m_points.z());
  return true;
}

bool Cube::setLimits(const double min[3], const double max[3],
                     const int points[3])
{
  return this->setLimits(Vector3d(min), Vector3d(max), Vector3i(points));
}

bool Cube::setLimits(const Vector3d &min, const Vector3d &max,
                     double spacing)
{
  Vector3i points;
  Vector3d delta = max - min;
  delta = delta / spacing;
  points = Vector3i(delta.x(), delta.y(), delta.z());
  return setLimits(min, max, points);
}

bool Cube::setLimits(const double min[3], const double max[3],
                     double spacing)
{
  return this->setLimits(Vector3d(min), Vector3d(max), spacing);
}

bool Cube::setLimits(const Vector3d &min, const Vector3i &dim,
                     double spacing)
{
  Vector3d max = Vector3d(min.x() + (dim.x()-1) * spacing,
                          min.y() + (dim.y()-1) * spacing,
                          min.z() + (dim.z()-1) * spacing);
  m_min = min;
  m_max = max;
  m_points = dim;
  m_spacing = Vector3d(spacing, spacing, spacing);
  m_data.resize(m_points.x() * m_points.y() * m_points.z());
  return true;
}

bool Cube::setLimits(const double min[3], const int dim[3],
                     double spacing)
{
  return this->setLimits(Vector3d(min), Vector3i(dim), spacing);
}

bool Cube::setLimits(const Cube &cube)
{
  m_min = cube.m_min;
  m_max = cube.m_max;
  m_points = cube.m_points;
  m_spacing = cube.m_spacing;
  m_data.resize(m_points.x() * m_points.y() * m_points.z());
  return true;
}

bool Cube::setLimits(const Molecule &mol, double spacing, double padding)
{
  size_t numAtoms = mol.numAtoms();
  Vector3d min, max;
  if (numAtoms) {
    Vector3d curPos = min = max = mol.atomPos(0);
    for (size_t i = 1; i < numAtoms; ++i) {
      curPos = mol.atomPos(i);
      if (curPos.x() < min.x()) min.x() = curPos.x();
      if (curPos.x() > max.x()) max.x() = curPos.x();
      if (curPos.y() < min.y()) min.y() = curPos.y();
      if (curPos.y() > max.y()) max.y() = curPos.y();
      if (curPos.z() < min.z()) min.z() = curPos.z();
      if (curPos.z() > max.z()) max.z() = curPos.z();
    }
  } else {
    min = max = Eigen::Vector3d::Zero();
  }

  // Now to take care of the padding term
  min += Vector3d(-padding, -padding, -padding);
  max += Vector3d(padding, padding, padding);

  return setLimits(min, max, spacing);
}

std::vector<double> * Cube::data()
{
  return &m_data;
}

bool Cube::setData(const std::vector<double> &values)
{
  if (!values.size()) {
    qDebug() << "Zero sized vector passed to Cube::setData. Nothing to do.";
    return false;
  }
  if (static_cast<int>(values.size()) == m_points.x() * m_points.y() * m_points.z()) {
    m_data = values;
    qDebug() << "Loaded in cube data" << m_data.size();
    // Now to update the minimum and maximum values
    m_minValue = m_maxValue = m_data[0];
    foreach(double val, m_data) {
      if (val < m_minValue)
        m_minValue = val;
      else if (val > m_maxValue)
        m_maxValue = val;
    }
    return true;
  }
  else {
    qDebug() << "The vector passed to Cube::setData does not have the correct"
             << "size. Expected" << m_points.x() * m_points.y() * m_points.z()
             << "got" << values.size();
    return false;
  }
}

bool Cube::addData(const std::vector<double> &values)
{
  // Initialise the cube to zero if necessary
  if (!m_data.size()) {
    m_data.resize(m_points.x() * m_points.y() * m_points.z());
  }
  if (values.size() != m_data.size() || !values.size()) {
    qDebug() << "Attempted to add values to cube - sizes do not match...";
    return false;
  }
  for (unsigned int i = 0; i < m_data.size(); i++) {
    m_data[i] += values[i];
    if (m_data[i] < m_minValue)
      m_minValue = m_data[i];
    else if (m_data[i] > m_maxValue)
      m_maxValue = m_data[i];
  }
  return true;
}

unsigned int Cube::closestIndex(const Vector3d &pos) const
{
  int i, j, k;
  // Calculate how many steps each coordinate is along its axis
  i = int((pos.x() - m_min.x()) / m_spacing.x());
  j = int((pos.y() - m_min.y()) / m_spacing.y());
  k = int((pos.z() - m_min.z()) / m_spacing.z());
  return i*m_points.y()*m_points.z() + j*m_points.z() + k;
}

unsigned int Cube::closestIndex(const double pos[3]) const
{
  return this->closestIndex(Vector3d(pos));
}

Vector3i Cube::indexVector(const Vector3d &pos) const
{
  // Calculate how many steps each coordinate is along its axis
  int i, j, k;
  i = int((pos.x() - m_min.x()) / m_spacing.x());
  j = int((pos.y() - m_min.y()) / m_spacing.y());
  k = int((pos.z() - m_min.z()) / m_spacing.z());
  return Vector3i(i, j, k);
}

void Cube::indexVector(const double pos[3], int ind[3]) const
{
  Vector3i eind = this->indexVector(Vector3d(pos));
  ind[0] = eind.x();
  ind[1] = eind.y();
  ind[2] = eind.z();
}

Vector3d Cube::position(unsigned int index) const
{
  int x, y, z;
  x = int(index / (m_points.y()*m_points.z()));
  y = int((index - (x*m_points.y()*m_points.z())) / m_points.z());
  z = index % m_points.z();
  return Vector3d(x * m_spacing.x() + m_min.x(),
                  y * m_spacing.y() + m_min.y(),
                  z * m_spacing.z() + m_min.z());
}

double Cube::value(int i, int j, int k) const
{
  unsigned int index = i*m_points.y()*m_points.z() + j*m_points.z() + k;
  if (index < m_data.size())
    return m_data[index];
  else {
    //      qDebug() << "Attempt to identify out of range index" << index << m_data.size();
    return 0.0;
  }
}

double Cube::value(const Vector3i &pos) const
{
  unsigned int index = pos.x()*m_points.y()*m_points.z() +
      pos.y()*m_points.z() +
      pos.z();
  if (index < m_data.size())
    return m_data[index];
  else {
    qDebug() << "Attempted to access an index out of range.";
    return 6969.0;
  }
}

float Cube::valuef(const Vector3f &pos) const
{
  // This is a really expensive operation and so should be avoided
  // Interpolate the value at the supplied vector - trilinear interpolation...
  Vector3f delta = pos - m_min.cast<float>();
  // Find the integer low and high corners
  Vector3i lC(delta.x() / m_spacing.x(),
              delta.y() / m_spacing.y(),
              delta.z() / m_spacing.z());
  Vector3i hC(lC.x() + 1,
              lC.y() + 1,
              lC.z() + 1);
  // So there are six corners in total - work out the delta of the position
  // and the low corner
  Vector3f P((delta.x() - lC.x()*m_spacing.x()) / m_spacing.x(),
             (delta.y() - lC.y()*m_spacing.y()) / m_spacing.y(),
             (delta.z() - lC.z()*m_spacing.z()) / m_spacing.z());
  Vector3f dP = Vector3f(1.0, 1.0, 1.0) - P;
  // Now calculate and return the interpolated value
  return value(lC.x(), lC.y(), lC.z()) * dP.x() * dP.y() * dP.z() +
      value(hC.x(), lC.y(), lC.z()) * P.x()  * dP.y() * dP.z() +
      value(lC.x(), hC.y(), lC.z()) * dP.x() * P.y()  * dP.z() +
      value(lC.x(), lC.y(), hC.z()) * dP.x() * dP.y() * P.z()  +
      value(hC.x(), lC.y(), hC.z()) * P.x()  * dP.y() * P.z()  +
      value(lC.x(), hC.y(), hC.z()) * dP.x() * P.y()  * P.z()  +
      value(hC.x(), hC.y(), lC.z()) * P.x()  * P.y()  * dP.z() +
      value(hC.x(), hC.y(), hC.z()) * P.x()  * P.y()  * P.z();
}

float Cube::valuef(const float pos[3]) const
{
  return this->valuef(Vector3f(pos));
}

double Cube::value(const Vector3d &pos) const
{
  // This is a really expensive operation and so should be avoided
  // Interpolate the value at the supplied vector - trilinear interpolation...
  Vector3d delta = pos - m_min;
  // Find the integer low and high corners
  Vector3i lC(delta.x() / m_spacing.x(),
              delta.y() / m_spacing.y(),
              delta.z() / m_spacing.z());
  Vector3i hC(lC.x() + 1,
              lC.y() + 1,
              lC.z() + 1);
  // So there are six corners in total - work out the delta of the position
  // and the low corner
  Vector3d P((delta.x() - lC.x()*m_spacing.x()) / m_spacing.x(),
             (delta.y() - lC.y()*m_spacing.y()) / m_spacing.y(),
             (delta.z() - lC.z()*m_spacing.z()) / m_spacing.z());
  Vector3d dP = Vector3d(1.0, 1.0, 1.0) - P;
  // Now calculate and return the interpolated value
  return value(lC.x(), lC.y(), lC.z()) * dP.x() * dP.y() * dP.z() +
      value(hC.x(), lC.y(), lC.z()) * P.x()  * dP.y() * dP.z() +
      value(lC.x(), hC.y(), lC.z()) * dP.x() * P.y()  * dP.z() +
      value(lC.x(), lC.y(), hC.z()) * dP.x() * dP.y() * P.z()  +
      value(hC.x(), lC.y(), hC.z()) * P.x()  * dP.y() * P.z()  +
      value(lC.x(), hC.y(), hC.z()) * dP.x() * P.y()  * P.z()  +
      value(hC.x(), hC.y(), lC.z()) * P.x()  * P.y()  * dP.z() +
      value(hC.x(), hC.y(), hC.z()) * P.x()  * P.y()  * P.z();
}

double Cube::value(const double pos[3]) const
{
  return this->value(Vector3d(pos));
}

bool Cube::setValue(int i, int j, int k, double value)
{
  unsigned int index = i*m_points.y()*m_points.z() + j*m_points.z() + k;
  if (index < m_data.size()) {
    m_data[index] = value;
    return true;
  }
  else
    return false;
}

void Cube::setName(const char *name)
{
  this->setName(QString(name));
}

const char * Cube::name_c_str() const
{
  return this->name().toStdString().c_str();
}

QReadWriteLock * Cube::lock() const
{
  return m_lock;
}

} // End namespace

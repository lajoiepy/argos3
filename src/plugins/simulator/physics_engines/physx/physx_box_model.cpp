/**
 * @file <argos3/plugins/simulator/physics_engines/physx/physx_box_model.h>
 *
 * @author Carlo Pinciroli - <ilpincy@gmail.com>
 */

#include "physx_box_model.h"

namespace argos {

   /****************************************/
   /****************************************/

   CPhysXBoxModel::CPhysXBoxModel(CPhysXEngine& c_engine,
                                  CBoxEntity& c_entity) :
      CPhysXStretchableObjectModel(c_engine, c_entity) {
      /* Get the half size of the entity */
      physx::PxVec3 cHalfSize(m_cEntity.GetSize().GetY() * 0.5f,
                              m_cEntity.GetSize().GetZ() * 0.5f,
                              m_cEntity.GetSize().GetX() * 0.5f);
      m_cBaseCenterLocal.x = 0.0f;
      m_cBaseCenterLocal.y = -cHalfSize.y;
      m_cBaseCenterLocal.z = 0.0f;
      /* Get position and orientation in this engine's representation */
      physx::PxVec3 cPos;
      CVector3ToPxVec3(GetEmbodiedEntity().GetPosition(), cPos);
      physx::PxQuat cOrient;
      CQuaternionToPxQuat(GetEmbodiedEntity().GetOrientation(), cOrient);
      /* Create the transform
       * 1. a translation up by half size on y
       * 2. a rotation around the box base
       * 3. a translation to the final position
       */
      physx::PxTransform cTranslation1(physx::PxVec3(0.0f, cHalfSize.y, 0.0f));
      physx::PxTransform cRotation(cOrient);
      physx::PxTransform cTranslation2(cPos);
      physx::PxTransform cFinalTrans = cTranslation2 * cRotation * cTranslation1;
      /* Create the box geometry */
      physx::PxBoxGeometry cBoxGeom(cHalfSize);
      /* Create the box body */
      if(GetEmbodiedEntity().IsMovable()) {
         /*
          * The box is movable
          */
         /* Create the body in its initial position and orientation */
         m_pcDynamicBody = GetPhysXEngine().GetPhysics().createRigidDynamic(cFinalTrans);
         /* Create the shape */
         m_pcShape = m_pcDynamicBody->createShape(cBoxGeom,
                                                  GetPhysXEngine().GetDefaultMaterial());
         /* Switch continuous collision detection (CCD) on */
         m_pcShape->setFlag(physx::PxShapeFlag::eUSE_SWEPT_BOUNDS, true);
         /* Set body mass */
         physx::PxRigidBodyExt::setMassAndUpdateInertia(*m_pcDynamicBody, m_fMass);
         /* Add body to the scene */
         GetPhysXEngine().GetScene().addActor(*m_pcDynamicBody);
      }
      else {
         /*
          * The box is not movable
          */
         /* Create the body in its initial position and orientation */
         m_pcStaticBody = GetPhysXEngine().GetPhysics().createRigidStatic(cFinalTrans);
         /* Create the shape */
         m_pcShape = m_pcStaticBody->createShape(cBoxGeom,
                                                 GetPhysXEngine().GetDefaultMaterial());
         /* Add body to the scene */
         GetPhysXEngine().GetScene().addActor(*m_pcStaticBody);
      }
      /* Assign the user data pointer to this model */
      m_pcShape->userData = this;
      /* Calculate bounding box */
      CalculateBoundingBox();
   }

   /****************************************/
   /****************************************/

   REGISTER_STANDARD_PHYSX_OPERATIONS_ON_ENTITY(CBoxEntity, CPhysXBoxModel);

   /****************************************/
   /****************************************/

}

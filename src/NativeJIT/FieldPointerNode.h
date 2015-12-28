#pragma once

#include "Node.h"


namespace NativeJIT
{
    template <typename OBJECT, typename FIELD>
    class FieldPointerNode : public Node<FIELD*>
    {
    public:
        FieldPointerNode(ExpressionTree& tree, Node<OBJECT*>& base, FIELD OBJECT::*field);

        //
        // Overrides of Node methods
        //

        virtual ExpressionTree::Storage<FIELD*> CodeGenValue(ExpressionTree& tree) override;
        virtual unsigned LabelSubtree(bool isLeftChild) override;
        virtual void Print() const override;

        virtual void ReleaseReferencesToChildren() override;

    protected:
        virtual bool GetBaseAndOffset(NodeBase*& base, __int32& offset) const override;

    private:
        static __int32 Offset(FIELD OBJECT::*field)
        {
            return reinterpret_cast<__int32>(&((static_cast<OBJECT*>(nullptr))->*field));
        }

        NodeBase& m_base;
        const __int32 m_originalOffset;

        // Multiple accesses to the same base object can sometimes be collapsed
        // as an optimization. In such cases, m_collapsedBase/Offset will point
        // to such base object. Otherwise, they will match the base object/offset
        // from the constructor.
        // IMPORTANT: the constructor depends on collapsed base/offset being
        // listed after the original base/offset.
        NodeBase* m_collapsedBase;
        __int32 m_collapsedOffset;
    };


    //*************************************************************************
    //
    // Template definitions for FieldPointerNode
    //
    //*************************************************************************
    template <typename OBJECT, typename FIELD>
    FieldPointerNode<OBJECT, FIELD>::FieldPointerNode(ExpressionTree& tree,
                                                      Node<OBJECT*>& base,
                                                      FIELD OBJECT::*field)
        : Node(tree),
          m_base(base),
          m_originalOffset(Offset(field)),
          // Note: there is constructor order dependency for these two.
          m_collapsedBase(&m_base),
          m_collapsedOffset(m_originalOffset)
    {
        NodeBase* grandparent;
        __int32 parentOffset;

        // If base can be represented off of another object with an added offset,
        // make the reference off of that object and adjust the offset.
        if (base.GetBaseAndOffset(grandparent, parentOffset))
        {
            m_collapsedBase = grandparent;
            m_collapsedOffset += parentOffset;
            base.MarkReferenced();
        }

        m_collapsedBase->IncrementParentCount();
    }


    template <typename OBJECT, typename FIELD>
    typename ExpressionTree::Storage<FIELD*> FieldPointerNode<OBJECT, FIELD>::CodeGenValue(ExpressionTree& tree)
    {
        auto base = m_collapsedBase->CodeGenAsBase(tree);

        if (m_collapsedOffset == 0)
        {
            base.ConvertToDirect(false);
        }
        else
        {
            base.ConvertToDirect(true);
            tree.GetCodeGenerator()
                .EmitImmediate<OpCode::Add>(base.GetDirectRegister(), m_collapsedOffset);
        }

        // With the added offset, the type changes from void* to FIELD*.
        return ExpressionTree::Storage<FIELD*>(base);
    }


    template <typename OBJECT, typename FIELD>
    bool FieldPointerNode<OBJECT, FIELD>::GetBaseAndOffset(NodeBase*& base, __int32& offset) const
    {
        base = m_collapsedBase;
        offset = m_collapsedOffset;

        return true;
    }


    template <typename OBJECT, typename FIELD>
    void FieldPointerNode<OBJECT, FIELD>::ReleaseReferencesToChildren()
    {
        m_collapsedBase->DecrementParentCount();
    }


    template <typename OBJECT, typename FIELD>
    unsigned FieldPointerNode<OBJECT, FIELD>::LabelSubtree(bool /*isLeftChild*/)
    {
        // TODO: Should isLeftChild be passed down?
        SetRegisterCount(m_collapsedBase->LabelSubtree(true));
        return GetRegisterCount();
    }


    template <typename OBJECT, typename FIELD>
    void FieldPointerNode<OBJECT, FIELD>::Print() const
    {
        PrintCoreProperties("FieldPointerNode");

        std::cout << ", base ID = " << m_base.GetId()
                  << ", offset = " << m_originalOffset;

        if (m_base.GetId() != m_collapsedBase->GetId())
        {
            std::cout
                  << ", collapsed base ID = " << m_collapsedBase->GetId()
                  << ", collapsed offset = " << m_collapsedOffset;
        }
    }
}

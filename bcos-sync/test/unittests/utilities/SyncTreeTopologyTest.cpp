/*
 * @CopyRight:
 * FISCO-BCOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FISCO-BCOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FISCO-BCOS.  If not, see <http://www.gnu.org/licenses/>
 * (c) 2016-2019 fisco-dev contributors.
 */

/**
 * @brief : SyncTreeTopologyTest
 * @author: yujiechen
 * @date: 2023-3-27
 */
#include <bcos-crypto/interfaces/crypto/CommonType.h>
#include <bcos-crypto/signature/sm2/SM2KeyPairFactory.h>
#include <bcos-sync/utilities/SyncTreeTopology.h>
#include <bcos-utilities/Ranges.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace bcos::sync;

namespace bcos
{
namespace test
{
BOOST_FIXTURE_TEST_SUITE(SyncTreeTopologyTest, TestPromptFixture)

class FakeSyncTreeTopology : public SyncTreeTopology
{
public:
    using Ptr = std::shared_ptr<FakeSyncTreeTopology>;
    FakeSyncTreeTopology(bcos::crypto::NodeIDPtr _nodeId, std::uint32_t const _treeWidth = 2)
      : SyncTreeTopology(_nodeId, _treeWidth)
    {}

    virtual bcos::crypto::NodeIDs const& currentConsensusNodes() { return *m_consensusNodes; }

    virtual bcos::crypto::NodeIDs const& nodeList() { return *m_nodeList; }
    void recursiveSelectChildNodesWrapper(bcos::crypto::NodeIDListPtr selectedNodeList,
        ssize_t const parentIndex, bcos::crypto::NodeIDSetPtr peers)
    {
        std::int64_t offset = m_startIndex - 1;
        if (m_consIndex > 0)
        {
            return SyncTreeTopology::recursiveSelectChildNodes(selectedNodeList, 0, peers, offset);
        }
        else
        {
            std::int64_t nodeIndex = parentIndex + 1 - m_startIndex;
            return SyncTreeTopology::recursiveSelectChildNodes(
                selectedNodeList, nodeIndex, peers, offset);
        }
    }
    // select the parent nodes by tree
    void selectParentNodesWrapper(bcos::crypto::NodeIDListPtr selectedNodeList,
        bcos::crypto::NodeIDSetPtr peers, std::int64_t const _nodeIndex)
    {
        std::int64_t offset = m_startIndex - 1;
        std::int64_t nodeIndex = _nodeIndex + 1 - m_startIndex;
        return SyncTreeTopology::selectParentNodes(selectedNodeList, peers, nodeIndex, offset);
    }
    virtual std::int64_t nodeNum() { return m_nodeNum; }

    virtual bcos::crypto::NodeIDPtr nodeId() { return m_nodeId; }

    virtual std::int64_t nodeIndex() const { return m_nodeIndex; }
    virtual std::int64_t consIndex() const { return m_consIndex; }
    virtual std::int64_t endIndex() const { return m_endIndex; }
    virtual std::int64_t startIndex() const { return m_startIndex; }
};

class SyncTreeToplogyFixture
{
public:
    using Ptr = std::shared_ptr<SyncTreeToplogyFixture>;
    SyncTreeToplogyFixture(std::uint32_t const _treeWidth = 2)
    {
        bcos::crypto::KeyPairFactory::Ptr keyPairFactory =
            std::make_shared<bcos::crypto::SM2KeyPairFactory>();
        bcos::crypto::KeyPairInterface::Ptr keyPair = keyPairFactory->generateKeyPair();
        // create a 2-tree rounter
        m_syncTreeRouter = std::make_shared<FakeSyncTreeTopology>(keyPair->publicKey(), _treeWidth);
        m_peers = std::make_shared<bcos::crypto::NodeIDSet>();
    }

    void addNodeIntoConsensusList(bcos::crypto::NodeIDPtr _nodeId)
    {
        m_nodeList.push_back(_nodeId);
        m_consensusList.push_back(_nodeId);
        m_peers->insert(_nodeId);
    }

    void addNodeIntoObserverList(bcos::crypto::NodeIDPtr _nodeId)
    {
        m_nodeList.push_back(_nodeId);
        m_peers->insert(_nodeId);
    }

    // fake ConsensusList
    void fakeConsensusList(std::size_t _size)
    {
        bcos::crypto::KeyPairFactory::Ptr keyPairFactory =
            std::make_shared<bcos::crypto::SM2KeyPairFactory>();
        for (std::size_t i = 0; i < _size; ++i)
        {
            bcos::crypto::KeyPairInterface::Ptr nodeId = keyPairFactory->generateKeyPair();
            m_consensusList.push_back(nodeId->publicKey());
            m_nodeList.push_back(nodeId->publicKey());
            m_peers->insert(nodeId->publicKey());
        }
    }

    // fake observerList
    void fakeObserverList(std::size_t _nodeId)
    {
        bcos::crypto::KeyPairFactory::Ptr keyPairFactory =
            std::make_shared<bcos::crypto::SM2KeyPairFactory>();
        for (std::size_t i = 0; i < _nodeId; ++i)
        {
            bcos::crypto::KeyPairInterface::Ptr nodeId = keyPairFactory->generateKeyPair();
            m_nodeList.push_back(nodeId->publicKey());
            m_peers->insert(nodeId->publicKey());
        }
    }

    // clear both nodeList and consensusList
    void clearNodeList()
    {
        m_consensusList.clear();
        m_nodeList.clear();
        m_peers->clear();
    }

    void updateNodeListInfo() { m_syncTreeRouter->updateNodeInfo(m_nodeList); }

    void updateConsensusNodeInfo()
    {
        m_syncTreeRouter->updateAllNodeInfo(m_consensusList, m_nodeList);
    }
    std::shared_ptr<FakeSyncTreeTopology> syncTreeRouter() { return m_syncTreeRouter; }
    bcos::crypto::NodeIDs const& consensusList() { return m_consensusList; }
    bcos::crypto::NodeIDs const& nodeList() { return m_nodeList; }
    bcos::crypto::NodeIDSetPtr peers() { return m_peers; }

private:
    std::shared_ptr<FakeSyncTreeTopology> m_syncTreeRouter = nullptr;
    bcos::crypto::NodeIDs m_consensusList;  // faked consensus list
    bcos::crypto::NodeIDs m_nodeList;       // faked node list
    bcos::crypto::NodeIDSetPtr m_peers;
};

void checkSelectedNodes(SyncTreeToplogyFixture::Ptr fakeSyncTreeTopology,
    bcos::crypto::NodeIDs const& _selectedNodes, std::vector<std::size_t> _idxVec)
{
    BOOST_CHECK(_selectedNodes.size() == _idxVec.size());
    for (auto const& [idx, node] : _selectedNodes | RANGES::views::enumerate)
    {
        BOOST_CHECK(node->data() ==
                    fakeSyncTreeTopology->syncTreeRouter()->nodeList()[_idxVec[idx]]->data());
    }
}

/// case1: the node doesn't locate in the node list (free node)
BOOST_AUTO_TEST_CASE(testFreeNode)
{
    SyncTreeToplogyFixture::Ptr fakeSyncTreeTopology = std::make_shared<SyncTreeToplogyFixture>();

    fakeSyncTreeTopology->fakeConsensusList(2);

    // updateConsensusNodeInfo and check consensusList
    fakeSyncTreeTopology->updateConsensusNodeInfo();
    BOOST_CHECK(true == bcos::crypto::KeyCompareTools::compareTwoNodeIDs(
                            fakeSyncTreeTopology->consensusList(),
                            fakeSyncTreeTopology->syncTreeRouter()->currentConsensusNodes()));
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->consIndex() == -1);
    // check node list
    BOOST_CHECK(true == bcos::crypto::KeyCompareTools::compareTwoNodeIDs(
                            fakeSyncTreeTopology->syncTreeRouter()->nodeList(),
                            fakeSyncTreeTopology->nodeList()));
    // check startIndex and endIndex
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->startIndex() == 0);
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->endIndex() == 0);

    // updateNodeInfo and check NodeInfo
    fakeSyncTreeTopology->updateNodeListInfo();
    BOOST_CHECK(
        true == bcos::crypto::KeyCompareTools::compareTwoNodeIDs(fakeSyncTreeTopology->nodeList(),
                    fakeSyncTreeTopology->syncTreeRouter()->nodeList()));
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->nodeNum() ==
                (std::int64_t)fakeSyncTreeTopology->nodeList().size());
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->nodeIndex() == -1);
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->consIndex() == -1);
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->startIndex() == 0);
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->endIndex() == 0);

    // selectNodes: no need to send blocks to any nodes
    bcos::crypto::NodeIDListPtr selectedNodeList = std::make_shared<bcos::crypto::NodeIDs>();
    fakeSyncTreeTopology->syncTreeRouter()->recursiveSelectChildNodesWrapper(
        selectedNodeList, -1, fakeSyncTreeTopology->peers());
    BOOST_CHECK(true == selectedNodeList->empty());
}

/// case2: the node locates in the observerList
BOOST_AUTO_TEST_CASE(testForTheObserverNode)
{
    SyncTreeToplogyFixture::Ptr fakeSyncTreeTopology = std::make_shared<SyncTreeToplogyFixture>();

    fakeSyncTreeTopology->fakeConsensusList(2);
    fakeSyncTreeTopology->updateConsensusNodeInfo();
    fakeSyncTreeTopology->updateNodeListInfo();

    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->consIndex() == -1);
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->nodeIndex() == -1);

    // push the node into the observerList
    /// case1: nodeIndex is 2, no need to broadcast to any node
    fakeSyncTreeTopology->addNodeIntoObserverList(fakeSyncTreeTopology->syncTreeRouter()->nodeId());
    fakeSyncTreeTopology->updateNodeListInfo();
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->consIndex() == -1);
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->nodeIndex() == 2);
    // check NodeList
    BOOST_CHECK(
        true == bcos::crypto::KeyCompareTools::compareTwoNodeIDs(fakeSyncTreeTopology->nodeList(),
                    fakeSyncTreeTopology->syncTreeRouter()->nodeList()));
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->nodeNum() ==
                (std::int64_t)fakeSyncTreeTopology->nodeList().size());

    // select target nodes from peers for this node
    bcos::crypto::NodeIDListPtr selectedNodeList = std::make_shared<bcos::crypto::NodeIDs>();
    fakeSyncTreeTopology->syncTreeRouter()->recursiveSelectChildNodesWrapper(
        selectedNodeList, 2, fakeSyncTreeTopology->peers());
    BOOST_CHECK(true == selectedNodeList->empty());

    // select parent for the 2th nodes
    selectedNodeList->clear();
    fakeSyncTreeTopology->syncTreeRouter()->selectParentNodesWrapper(
        selectedNodeList, fakeSyncTreeTopology->peers(), 1);
    BOOST_CHECK(true == selectedNodeList->empty());

    /// case2: nodeIndex is 0
    fakeSyncTreeTopology->clearNodeList();
    fakeSyncTreeTopology->addNodeIntoObserverList(fakeSyncTreeTopology->syncTreeRouter()->nodeId());
    fakeSyncTreeTopology->fakeObserverList(10);
    fakeSyncTreeTopology->fakeConsensusList(2);
    fakeSyncTreeTopology->updateConsensusNodeInfo();
    fakeSyncTreeTopology->updateNodeListInfo();
    BOOST_CHECK(true == bcos::crypto::KeyCompareTools::compareTwoNodeIDs(
                            fakeSyncTreeTopology->syncTreeRouter()->nodeList(),
                            fakeSyncTreeTopology->nodeList()));
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->nodeNum() ==
                (std::int64_t)fakeSyncTreeTopology->nodeList().size());
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->nodeIndex() == 0);
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->consIndex() == -1);
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->startIndex() == 0);
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->endIndex() == 7);

    // check and select nodes for the 0th node
    selectedNodeList->clear();
    fakeSyncTreeTopology->syncTreeRouter()->recursiveSelectChildNodesWrapper(
        selectedNodeList, 0, fakeSyncTreeTopology->peers());
    std::vector<std::size_t> idxVec = {2, 3};
    checkSelectedNodes(fakeSyncTreeTopology, *selectedNodeList, idxVec);

    // check and select nodes for the 1th node
    selectedNodeList->clear();
    fakeSyncTreeTopology->syncTreeRouter()->recursiveSelectChildNodesWrapper(
        selectedNodeList, 1, fakeSyncTreeTopology->peers());
    idxVec = {4, 5};
    checkSelectedNodes(fakeSyncTreeTopology, *selectedNodeList, idxVec);

    // check and select nodes for the 2th node
    selectedNodeList->clear();
    fakeSyncTreeTopology->syncTreeRouter()->recursiveSelectChildNodesWrapper(
        selectedNodeList, 2, fakeSyncTreeTopology->peers());
    idxVec = {6};
    checkSelectedNodes(fakeSyncTreeTopology, *selectedNodeList, idxVec);

    // check other nodes that has no child
    for (std::size_t i = 3; i <= 6; ++i)
    {
        selectedNodeList->clear();
        fakeSyncTreeTopology->syncTreeRouter()->recursiveSelectChildNodesWrapper(
            selectedNodeList, i, fakeSyncTreeTopology->peers());
        checkSelectedNodes(fakeSyncTreeTopology, *selectedNodeList, {});
    }
    /// check parent
    for (std::size_t i = 0; i <= 1; ++i)
    {
        selectedNodeList->clear();
        fakeSyncTreeTopology->syncTreeRouter()->selectParentNodesWrapper(
            selectedNodeList, fakeSyncTreeTopology->peers(), i);
        // BOOST_CHECK(true == selectedNodeList->empty());
    }
    // check parent for 2th node
    for (std::size_t i = 2; i <= 2; ++i)
    {
        selectedNodeList->clear();
        fakeSyncTreeTopology->syncTreeRouter()->selectParentNodesWrapper(
            selectedNodeList, fakeSyncTreeTopology->peers(), i);
        checkSelectedNodes(fakeSyncTreeTopology, *selectedNodeList, {});
    }
    for (std::size_t i = 3; i <= 3; ++i)
    {
        selectedNodeList->clear();
        fakeSyncTreeTopology->syncTreeRouter()->selectParentNodesWrapper(
            selectedNodeList, fakeSyncTreeTopology->peers(), i);
        checkSelectedNodes(fakeSyncTreeTopology, *selectedNodeList, {1});
    }

    for (std::size_t i = 4; i <= 4; ++i)
    {
        selectedNodeList->clear();
        fakeSyncTreeTopology->syncTreeRouter()->selectParentNodesWrapper(
            selectedNodeList, fakeSyncTreeTopology->peers(), i);
        checkSelectedNodes(fakeSyncTreeTopology, *selectedNodeList, {1});
    }
    for (std::size_t i = 5; i <= 5; ++i)
    {
        selectedNodeList->clear();
        fakeSyncTreeTopology->syncTreeRouter()->selectParentNodesWrapper(
            selectedNodeList, fakeSyncTreeTopology->peers(), i);
        checkSelectedNodes(fakeSyncTreeTopology, *selectedNodeList, {2});
    }
    selectedNodeList->clear();
    fakeSyncTreeTopology->syncTreeRouter()->selectParentNodesWrapper(
        selectedNodeList, fakeSyncTreeTopology->peers(), 6);
    checkSelectedNodes(fakeSyncTreeTopology, *selectedNodeList, {2});


    /// case3: nodeIndex is 13
    fakeSyncTreeTopology->clearNodeList();
    fakeSyncTreeTopology->fakeObserverList(10);
    fakeSyncTreeTopology->fakeConsensusList(2);
    fakeSyncTreeTopology->addNodeIntoObserverList(fakeSyncTreeTopology->syncTreeRouter()->nodeId());
    fakeSyncTreeTopology->updateConsensusNodeInfo();
    fakeSyncTreeTopology->updateNodeListInfo();

    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->nodeIndex() == 12);
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->consIndex() == -1);
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->startIndex() == 7);
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->endIndex() == 6);

    // check the 7th nodes
    selectedNodeList->clear();
    fakeSyncTreeTopology->syncTreeRouter()->recursiveSelectChildNodesWrapper(
        selectedNodeList, 7, fakeSyncTreeTopology->peers());
    idxVec = {9, 10};
    checkSelectedNodes(fakeSyncTreeTopology, *selectedNodeList, idxVec);
    // check parent for 9th and 10th
    for (std::size_t i = 9; i <= 9; ++i)
    {
        selectedNodeList->clear();
        fakeSyncTreeTopology->syncTreeRouter()->selectParentNodesWrapper(
            selectedNodeList, fakeSyncTreeTopology->peers(), i);
        checkSelectedNodes(fakeSyncTreeTopology, *selectedNodeList, {7});
    }
    for (std::size_t i = 10; i <= 10; ++i)
    {
        selectedNodeList->clear();
        fakeSyncTreeTopology->syncTreeRouter()->selectParentNodesWrapper(
            selectedNodeList, fakeSyncTreeTopology->peers(), i);
        checkSelectedNodes(fakeSyncTreeTopology, *selectedNodeList, {8});
    }

    // check the 8th nodes
    selectedNodeList->clear();
    fakeSyncTreeTopology->syncTreeRouter()->recursiveSelectChildNodesWrapper(
        selectedNodeList, 8, fakeSyncTreeTopology->peers());
    idxVec = {11};
    checkSelectedNodes(fakeSyncTreeTopology, *selectedNodeList, idxVec);
    for (std::size_t i = 11; i <= 11; ++i)
    {
        selectedNodeList->clear();
        fakeSyncTreeTopology->syncTreeRouter()->selectParentNodesWrapper(
            selectedNodeList, fakeSyncTreeTopology->peers(), i);
        checkSelectedNodes(fakeSyncTreeTopology, *selectedNodeList, {8});
    }
    for (std::size_t i = 12; i <= 12; ++i)
    {
        selectedNodeList->clear();
        fakeSyncTreeTopology->syncTreeRouter()->selectParentNodesWrapper(
            selectedNodeList, fakeSyncTreeTopology->peers(), i);
        checkSelectedNodes(fakeSyncTreeTopology, *selectedNodeList, {9});
    }
}

/// case3: the node locates in the consensus nodeList
BOOST_AUTO_TEST_CASE(testForTheConsensusNode)
{
    // init SyncTreeToplogyFixture
    SyncTreeToplogyFixture::Ptr fakeSyncTreeTopology = std::make_shared<SyncTreeToplogyFixture>(2);
    fakeSyncTreeTopology->fakeObserverList(11);
    fakeSyncTreeTopology->fakeConsensusList(1);
    fakeSyncTreeTopology->addNodeIntoConsensusList(
        fakeSyncTreeTopology->syncTreeRouter()->nodeId());
    fakeSyncTreeTopology->updateConsensusNodeInfo();
    fakeSyncTreeTopology->updateNodeListInfo();
    BOOST_CHECK(true == bcos::crypto::KeyCompareTools::compareTwoNodeIDs(
                            fakeSyncTreeTopology->syncTreeRouter()->nodeList(),
                            fakeSyncTreeTopology->nodeList()));
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->nodeNum() ==
                (std::int64_t)fakeSyncTreeTopology->nodeList().size());
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->nodeIndex() == 12);
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->consIndex() == 1);
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->startIndex() == 7);
    BOOST_CHECK(fakeSyncTreeTopology->syncTreeRouter()->endIndex() == 6);

    // get childNodes and parentNodes for this node
    bcos::crypto::NodeIDListPtr selectedNodeList = std::make_shared<bcos::crypto::NodeIDs>();
    fakeSyncTreeTopology->syncTreeRouter()->recursiveSelectChildNodesWrapper(
        selectedNodeList, 0, fakeSyncTreeTopology->peers());
    checkSelectedNodes(fakeSyncTreeTopology, *selectedNodeList, {7, 8});

    // check parent
    selectedNodeList->clear();
    fakeSyncTreeTopology->syncTreeRouter()->selectParentNodesWrapper(selectedNodeList,
        fakeSyncTreeTopology->peers(), fakeSyncTreeTopology->syncTreeRouter()->nodeIndex());
    selectedNodeList->emplace_back(fakeSyncTreeTopology->syncTreeRouter()->nodeId());
    BOOST_CHECK(
        *selectedNodeList == fakeSyncTreeTopology->syncTreeRouter()->currentConsensusNodes());
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace test
}  // namespace bcos
// 目录树展开折叠功能
function toggleDirectory(event, linkElement) {
    // 阻止默认的链接跳转行为
    event.preventDefault();
    event.stopPropagation();

    // 获取一级目录的li元素
    const parentLi = linkElement.parentElement;
    // 获取这个li下的所有二级ul元素
    const subUls = parentLi.querySelectorAll('ul');

    if (subUls.length > 0) {
        // 获取第一个二级ul的状态来决定是展开还是折叠
        const isCollapsed = subUls[0].classList.contains('collapsed');

        // 对所有二级ul应用相同的状态
        subUls.forEach(function (subUl) {
            if (isCollapsed) {
                // 展开
                subUl.classList.remove('collapsed');
                subUl.style.height = 'auto';
            } else {
                // 折叠
                subUl.classList.add('collapsed');
                subUl.style.height = '0';
            }
        });
    }
}

// 页面加载完成后初始化
document.addEventListener('DOMContentLoaded', function () {
    // 找到所有有子目录的一级目录链接
    const dirTree = document.querySelector('.dir-tree');
    if (dirTree) {
        const topLevelUls = dirTree.querySelectorAll(':scope > ul');

        topLevelUls.forEach(function (ul) {
            const li = ul.querySelector('li');
            if (li) {
                // 检查这个li是否包含子ul（即有子目录）
                const subUls = li.querySelectorAll('ul');
                if (subUls.length > 0) {
                    // 找到这个li中的第一个链接（一级目录标题）
                    const firstLink = li.querySelector(':scope > a');
                    if (firstLink) {
                        // 为这个链接添加点击事件
                        firstLink.addEventListener('click', function (event) {
                            toggleDirectory(event, this);
                        });

                        // 为一级目录链接添加样式标识
                        firstLink.style.cursor = 'pointer';
                        firstLink.style.userSelect = 'none';
                    }
                }
            }
        });
    }

    // 默认所有一级目录都是展开状态
    const subUls = document.querySelectorAll('.dir-tree ul ul');
    subUls.forEach(function (subUl) {
        subUl.classList.remove('collapsed');
        subUl.style.height = 'auto';
    });
}); 
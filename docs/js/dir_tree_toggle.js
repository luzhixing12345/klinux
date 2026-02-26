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


function initDirTreeToggle() {
    // 找到所有有子目录的一级目录链接
    const dirTree = document.querySelector('.dir-tree');
    if (dirTree) {
        const topLevelUls = dirTree.querySelectorAll(':scope > ul');

        topLevelUls.forEach(function (ul) {
            const li = ul.querySelector('li');
            if (li) {
                // 检查这个li是否包含子ul(即有子目录)
                const subUls = li.querySelectorAll('ul');
                if (subUls.length > 0) {
                    // 找到这个li中的第一个链接(一级目录标题)
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
        var links = dirTree.querySelectorAll("a");
        var currentUrl = window.location.href.slice(0, -1);
        links.forEach(function (link) {
            if (link.href === currentUrl) {
                // 检查这个链接是否是一级目录链接(即父元素li下面有ul子元素)
                const parentLi = link.parentElement;
                const hasSubUl = parentLi.querySelector('ul');

                // 只对非一级目录的链接(即文件链接)应用active样式
                if (!hasSubUl) {
                    link.scrollIntoView({ block: 'center', inline: 'nearest' });
                    link.classList.add("link-active");
                }
            }
        });
    } else {
        console.error("no dir tree");
    }

    // 默认所有一级目录都是展开状态
    const subUls = document.querySelectorAll('.dir-tree ul ul');
    subUls.forEach(function (subUl) {
        subUl.classList.remove('collapsed');
        subUl.style.height = 'auto';
    });
    document.dispatchEvent(new Event("DirTreeReady"));
}

var current_path = window.location.pathname;
// console.log(current_path);
var dir_tree_path = current_path + "../../../dir-tree.html";
var is_index = false;

// check if current_path is root path
// root path is like "/klinux/"
// not root path is like "'/klinux/articles/%E5%BF%AB%E9%80%9F%E5%BC%80%E5%A7%8B/%E7%BC%96%E8%AF%91%E5%86%85%E6%A0%B8/'"

// if '/' exist more than 2, it is not root path
if (current_path.match(/\//g).length === 2) {
    dir_tree_path = current_path + "/../dir-tree.html";
    is_index = true;
}

// console.log(dir_tree_path);
// read embed html body and move to body
fetch(dir_tree_path)
    .then(res => res.text())
    .then(html => {
        const placeholder =
            document.getElementById("dir-tree-placeholder");
        if (is_index) {
            // replace html template ../.. to ./articles
            html = html.replace(/\.\.\/\.\./g, "./articles");
        }
        placeholder.outerHTML = html;
        initDirTreeToggle();
    });


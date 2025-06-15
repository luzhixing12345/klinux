const divElement = document.getElementsByClassName("header-navigator")[0]; // 获取目标div元素

// 监听窗口尺寸变化

function handleResize() {
    var screenWidth = window.innerWidth || document.documentElement.clientWidth || document.body.clientWidth;

    if (screenWidth > 768) {

        divElement.style.display = "block"; // 将display属性设置为block,以显示元素

        let navigator_links = document.querySelectorAll('.header-navigator ul li a[href^="#"]');
        navigator_links.forEach(link => {
            link.addEventListener('click', function (event) {
                event.preventDefault();
                let target = document.querySelector(this.getAttribute('href'));
                target.scrollIntoView({ behavior: 'smooth', block: 'start', inline: 'nearest' });
                // 修改网页 URL
                let url = window.location.href.split('#')[0];
                let newUrl = url + this.getAttribute('href');
                history.pushState(null, null, newUrl);

            });
        });

        function isScrolledIntoView(elem) {
            var docViewTop = window.pageYOffset;
            var docViewBottom = docViewTop + window.innerHeight;
            var elemTop = elem.offsetTop;
            var elemBottom = elemTop + elem.offsetHeight;

            // 修改判定逻辑：
            // 1. 元素顶部在视图中
            // 2. 元素底部在视图中
            // 3. 元素完全包含视图
            return (
                (elemTop >= docViewTop && elemTop <= docViewBottom) ||
                (elemBottom >= docViewTop && elemBottom <= docViewBottom) ||
                (elemTop <= docViewTop && elemBottom >= docViewBottom)
            );
        }

        var headings = document.querySelectorAll('h1, h2, h3, h4, h5, h6');
        var previousHeading;
        var currentHeading;

        window.addEventListener('scroll', function () {
            var found = false;
            // 记录最接近视口顶部的可见标题
            var closestHeading = null;
            var closestDistance = Infinity;

            for (var heading of headings) {
                if (isScrolledIntoView(heading)) {
                    var distance = Math.abs(heading.getBoundingClientRect().top);
                    if (distance < closestDistance) {
                        closestDistance = distance;
                        closestHeading = heading;
                        found = true;
                    }
                }
            }

            // 重置所有链接的样式
            var allLinks = document.querySelectorAll('.header-navigator a');
            allLinks.forEach(link => link.style.fontWeight = 'normal');

            // 设置最近的标题链接为粗体
            if (found && closestHeading) {
                var link = document.querySelector(`a[href="#${closestHeading.id}"]`);
                if (link) {
                    // 根据当前主题设置颜色
                    var isDarkMode = document.querySelector('.markdown-body').classList.contains('markdown-dark');
                    link.style.color = isDarkMode ? 'white' : 'black';
                    link.style.fontWeight = '900';  // 使用更重的字重
                    currentHeading = closestHeading;
                }
            } else if (!found && currentHeading) {
                // 如果没有找到可见的标题，保持当前标题的高亮状态
                var link = document.querySelector(`a[href="#${currentHeading.id}"]`);
                if (link) {
                    // 根据当前主题设置颜色
                    var isDarkMode = document.querySelector('.markdown-body').classList.contains('markdown-dark');
                    link.style.color = isDarkMode ? 'white' : 'black';
                    link.style.fontWeight = '900';  // 使用更重的字重
                }
            }
        });
    } else {
        // 删除导航栏
        divElement.style.display = "none";
    }
}

handleResize()

window.addEventListener('resize', handleResize)